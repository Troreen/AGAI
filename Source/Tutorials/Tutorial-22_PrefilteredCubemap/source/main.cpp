// Tutorial 22: Prefiltered Cubemap & Image-Based Lighting Pipeline
// Demonstrates offline & runtime environment cubemap prefiltering for physically-based rendering:
// - Equirectangular / Cube Cross / Live Scene Capture -> Cubemap
// - Split-Sum specular GGX importance sampling & cosine diffuse irradiance convolution
// - Lys-compatible Burley mip roughness mapping
// - Export to BC6H HDR DDS textures

#include <fstream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>

#include <tge/Application.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/graphics/Camera.h>
#include <tge/graphics/AmbientLight.h>
#include <tge/graphics/DirectionalLight.h>
#include <tge/graphics/RenderTarget.h>
#include <tge/graphics/DepthBuffer.h>
#include <tge/graphics/FullscreenEffect.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/Model/ModelFactory.h>
#include <tge/Model/ModelInstance.h>
#include <tge/shaders/ModelShader.h>
#include <tge/input/InputManager.h>
#include <tge/Timer.h>
#include <tge/log/Log.h>
#include <tge/settings/settings.h>
#include <tge/texture/TextureManager.h>

#include "imgui/imgui.h"
#include "CubemapPrefilter.h"
#include "FileDialogs.h"

using namespace Tga;

float camSpeed = 1000.f;
float camRotSpeed = 1.f;

struct RenderData
{
    bool enableDirectionalLight = true;
    bool enableAmbientLight = true;
    bool enableTonemapping = true;
    bool showSkybox = true;
    int skyboxMipLevel = 0;
    float environmentIntensity = 1.0f;

    DepthBuffer intermediateDepth;
    RenderTarget intermediateTexture;

    std::vector<std::shared_ptr<ModelInstance>> models;
    std::shared_ptr<DirectionalLight> directionalLight;
    std::shared_ptr<AmbientLight> ambientLight;
    std::shared_ptr<Camera> mainCamera;
};

void DrawSceneModels(RenderData& renderData, GraphicsEngine& graphicsEngine)
{
    ModelDrawer& modelDrawer = graphicsEngine.GetModelDrawer();
    for (const auto& modelInstance : renderData.models)
    {
        modelDrawer.DrawPbr(*modelInstance);
    }
}

void RenderSkybox(RenderData& renderData, GraphicsEngine& graphicsEngine)
{
    if (!renderData.showSkybox || !renderData.ambientLight || !renderData.ambientLight->cubemap)
        return;

    const VertexShader* skyVS = DX11::LoadVertexShader("data/shaders/SkyboxVS");
    if (!skyVS || !skyVS->shader) skyVS = DX11::LoadVertexShader("Shaders/SkyboxVS");

    const PixelShader* skyPS = DX11::LoadPixelShader("data/shaders/SkyboxPS");
    if (!skyPS || !skyPS->shader) skyPS = DX11::LoadPixelShader("Shaders/SkyboxPS");

    if (!skyVS || !skyVS->shader || !skyPS || !skyPS->shader)
        return;

    GraphicsStateStack& stateStack = graphicsEngine.GetGraphicsStateStack();
    stateStack.Push();

    stateStack.SetBlendState(BlendState::Disabled);
    stateStack.SetDepthStencilState(DepthStencilState::ReadOnlyLessOrEqual);
    stateStack.SetRasterizerState(RasterizerState::NoFaceCulling);

    float skyboxIntensity = renderData.enableAmbientLight ? renderData.environmentIntensity : 0.0f;
    Vector4f customParams(static_cast<float>(renderData.skyboxMipLevel), skyboxIntensity, 0.0f, 0.0f);
    stateStack.SetCustomShaderParameters(customParams);
    stateStack.UpdateGpuStates();

    ID3D11ShaderResourceView* srv = renderData.ambientLight->cubemap->GetShaderResourceView();
    DX11::Context->PSSetShaderResources(0, 1, &srv);

    DX11::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DX11::Context->IASetInputLayout(nullptr);
    DX11::Context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    DX11::Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

    DX11::Context->VSSetShader(skyVS->shader.Get(), nullptr, 0);
    DX11::Context->GSSetShader(nullptr, nullptr, 0);
    DX11::Context->PSSetShader(skyPS->shader.Get(), nullptr, 0);

    DX11::Context->Draw(3, 0);
    DX11::LogDrawCall();

    stateStack.Pop();
}

void RenderScene(RenderData& renderData, GraphicsEngine& graphicsEngine)
{
    GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();

    if (renderData.enableDirectionalLight && renderData.directionalLight)
    {
        graphicsStateStack.SetDirectionalLight(*renderData.directionalLight);
    }
    else
    {
        DirectionalLight noLight{};
        noLight.color = Color{ 0.0f, 0.0f, 0.0f, 0.0f };
        graphicsStateStack.SetDirectionalLight(noLight);
    }

    if (renderData.enableAmbientLight && renderData.ambientLight)
    {
        AmbientLight currentAmbient = *renderData.ambientLight;
        float intensity = renderData.environmentIntensity;
        currentAmbient.color = Color::FromLinear(intensity, intensity, intensity);
        graphicsStateStack.SetAmbientLight(currentAmbient);
    }
    else
    {
        AmbientLight noAmbient{};
        noAmbient.color = Color{ 0.0f, 0.0f, 0.0f, 0.0f };
        noAmbient.type = AmbientLightType::Custom;
        noAmbient.cubemap = nullptr;
        graphicsStateStack.SetAmbientLight(noAmbient);
    }

    graphicsStateStack.ClearPointLights();

    graphicsStateStack.Push();
    graphicsStateStack.SetBlendState(BlendState::Disabled);
    RenderSkybox(renderData, graphicsEngine);
    DrawSceneModels(renderData, graphicsEngine);
    graphicsStateStack.Pop();
}

void Render(RenderData& renderData, GraphicsEngine& graphicsEngine)
{
    GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();

    renderData.intermediateTexture.Clear();
    renderData.intermediateDepth.Clear();

    graphicsStateStack.SetCamera(*renderData.mainCamera);
    renderData.intermediateTexture.SetAsActiveTarget(&renderData.intermediateDepth);

    RenderScene(renderData, graphicsEngine);

    graphicsStateStack.Push();
    graphicsStateStack.SetBlendState(BlendState::Disabled);
    DX11::BackBuffer->SetAsActiveTarget();
    renderData.intermediateTexture.SetAsResourceOnSlot(1);
    if (renderData.enableTonemapping)
    {
        graphicsEngine.GetFullscreenEffectTonemap().Render();
    }
    else
    {
        graphicsEngine.GetFullscreenEffectCopy().Render();
    }
    graphicsStateStack.Pop();
}

void Go(void);

int main(const int /*argc*/, const char* /*argv*/[])
{
    Go();
    return 0;
}

Tga::InputManager* SInputManager = nullptr;

void Go(void)
{
    Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
    Tga::ApplicationConfiguration& cfg = Tga::Settings::GetApplicationConfiguration();

    cfg.winProcCallback = [](HWND, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (SInputManager)
        {
            SInputManager->UpdateEvents(message, wParam, lParam);
        }
        return 0;
    };

    cfg.applicationName = L"TGE: Tutorial 22 - Prefiltered Cubemap & PBR Pipeline";
    cfg.activateDebugSystems = Tga::DebugFeature::Fps |
        Tga::DebugFeature::Mem |
        Tga::DebugFeature::Drawcalls |
        Tga::DebugFeature::Cpu |
        Tga::DebugFeature::Filewatcher |
        Tga::DebugFeature::OptimizeWarnings;
    cfg.enableVSync = true;

    if (!Tga::Application::Start() || !Tga::GraphicsEngine::Start())
    {
        ERROR_PRINT("Fatal error! Engine could not start!");
        system("pause");
        return;
    }

    Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();
    {
        Vector2ui resolution = Tga::Application::GetInstance()->GetRenderSize();
        HWND windowHandle = *Tga::Application::GetInstance()->GetHWND();

        Tga::InputManager inputManager(windowHandle);
        SInputManager = &inputManager;
        bool isMouseTrapped = false;

        RenderData renderData;
        renderData.intermediateDepth = DepthBuffer::Create(DX11::GetResolution());
        renderData.intermediateTexture = RenderTarget::Create(DX11::GetResolution(), DXGI_FORMAT_R32G32B32A32_FLOAT);

        ModelFactory& modelFactory = ModelFactory::GetInstance();

        std::shared_ptr<ModelInstance> mdlPlane = std::make_shared<ModelInstance>(modelFactory.GetModelInstance("Plane"));
        mdlPlane->GetTransform().Scale({ 15.0f });

        std::shared_ptr<ModelInstance> matball = std::make_shared<ModelInstance>(modelFactory.GetModelInstance("TMA_Matball.fbx"));
        matball->GetTransform().SetPosition({ 520.0f, 0.0f, 0.0f });
        matball->GetTransform().Rotate(Rotator(0, -35, 0));

        std::shared_ptr<ModelInstance> mdlChest = std::make_shared<ModelInstance>(modelFactory.GetModelInstance("Particle_Chest.fbx"));
        mdlChest->GetTransform().SetPosition({ -400.0f, 0.0f, 0.0f });
        mdlChest->GetTransform().Rotate(Rotator(0, 180, 0));

        // Roughness test balls (configured with textures from material_test.tgo)
        // Offset by +250 in X to account for the pivot at the first ball so the 6 columns center at X = 0
        std::shared_ptr<ModelInstance> roughnessBalls = std::make_shared<ModelInstance>(modelFactory.GetModelInstance("SM_RougnessBalls.fbx"));
        roughnessBalls->GetTransform().SetPosition({ 250.0f, 100.0f, 0.0f });
        roughnessBalls->GetTransform().Scale({ 2.0f });
        roughnessBalls->GetTransform().Rotate(Rotator(0, 90, 0));

        auto& textureManager = graphicsEngine.GetTextureManager();
        Texture* texBlack = textureManager.GetTexture("T_Black_c.dds");
        Texture* texWhite = textureManager.GetTexture("T_White_c.dds");
        Texture* texRoughness = textureManager.GetTexture("T_RoughnessValues_m.dds", TextureSrgbMode::ForceNoSrgbFormat);
        Texture* texMetallic = textureManager.GetTexture("T_MetallicValues_m.dds", TextureSrgbMode::ForceNoSrgbFormat);

        roughnessBalls->SetTexture(0, 0, texBlack);
        roughnessBalls->SetTexture(0, 2, texRoughness);
        roughnessBalls->SetTexture(1, 0, texWhite);
        roughnessBalls->SetTexture(1, 2, texRoughness);
        roughnessBalls->SetTexture(2, 0, texWhite);
        roughnessBalls->SetTexture(2, 2, texMetallic);

        renderData.models.push_back(mdlPlane);
        renderData.models.push_back(mdlChest);
        renderData.models.push_back(matball);
        renderData.models.push_back(roughnessBalls);

        float dirLightEuler[3] = { 225.0f, -45.0f, 0.0f };
        float dirLightColor[3] = { 1.0f, 0.9f, 0.8f };
        float dirLightIntensity = 1.0f;

        auto dLight = DirectionalLight{
            Matrix4x4f::CreateFromRollPitchYaw(Rotator(dirLightEuler[0], dirLightEuler[1], dirLightEuler[2])),
            Color::FromLinear(dirLightColor[0] * dirLightIntensity, dirLightColor[1] * dirLightIntensity, dirLightColor[2] * dirLightIntensity),
            0.1f
        };
        renderData.directionalLight = std::make_shared<DirectionalLight>(dLight);

        auto aLight = AmbientLight{
            Color{ 1.0f, 1.0f, 1.0f },
            AmbientLightType::Custom,
            nullptr
        };
        renderData.ambientLight = std::make_shared<AmbientLight>(aLight);

        std::shared_ptr<Camera> camera = std::make_shared<Camera>();
        camera->SetPerspectiveProjection(
            90.0f,
            { static_cast<float>(resolution.x), static_cast<float>(resolution.y) },
            0.1f,
            50000.0f);

        camera->GetTransform().SetPosition(Vector3f(0.0f, 500.0f, -550.0f));
        Rotator camRotation = Rotator(45, 0, 0);
        camera->GetTransform().Rotate(camRotation);
        renderData.mainCamera = camera;

        CubemapPrefilter prefilterSystem;
        prefilterSystem.Init();

        CubemapData baseCubemap;
        CubemapData prefilteredCubemap;

        std::string initialDDS = Settings::ResolveAssetPath("cube_1024_preblurred_Skansen_2026.dds");
        int outputResIndex = 2;
        const uint32_t outputResOptions[] = { 256, 512, 1024, 2048 };
        const char* outputResNames[] = { "256 x 256", "512 x 512", "1024 x 1024 (Default)", "2048 x 2048" };

        int sampleCountIndex = 2; // Default 1024 samples
        const int sampleCountOptions[] = { 512, 1024, 2048, 4096 };
        const char* sampleCountNames[] = { "512 Samples", "1024 Samples (Default)", "2048 Samples", "4096 Samples" };

        if (!initialDDS.empty())
        {
            if (prefilterSystem.LoadBaseFromDDS(initialDDS, baseCubemap))
            {
                renderData.ambientLight->cubemap = baseCubemap.resource.get();
            }
        }

        std::string statusMessage = "Ready. Loaded initial DDS cubemap.";
        bool statusIsError = false;

        Timer timer;
        bool bShouldRun = true;

        while (bShouldRun)
        {
            timer.Update();
            inputManager.Update();

            Matrix4x4f& camTransform = camera->GetTransform();
            Vector3f camMovement = Vector3f::Zero;

            if (isMouseTrapped)
            {
                if (inputManager.IsKeyHeld(0x57)) camMovement += camTransform.GetForward() * 1.0f;  // W
                if (inputManager.IsKeyHeld(0x53)) camMovement += camTransform.GetForward() * -1.0f; // S
                if (inputManager.IsKeyHeld(0x41)) camMovement += camTransform.GetRight() * -1.0f;   // A
                if (inputManager.IsKeyHeld(0x44)) camMovement += camTransform.GetRight() * 1.0f;    // D
                if (inputManager.IsKeyHeld(0x45)) camMovement += Vector3f(0, 1, 0) * 1.0f;          // E (Up)
                if (inputManager.IsKeyHeld(0x51)) camMovement += Vector3f(0, 1, 0) * -1.0f;         // Q (Down)

                camTransform.SetPosition(camTransform.GetPosition() + camMovement * camSpeed * timer.GetDeltaTime());

                const Vector2f mouseDelta = inputManager.GetMouseDelta();
                camRotation.X += camRotSpeed * mouseDelta.Y;
                camRotation.Y += camRotSpeed * mouseDelta.X;
                camTransform.SetRotation(camRotation);
            }

            if (inputManager.IsKeyPressed(VK_RBUTTON))
            {
                if (!isMouseTrapped)
                {
                    inputManager.HideMouse();
                    inputManager.CaptureMouse();
                    isMouseTrapped = true;
                }
            }

            if (inputManager.IsKeyReleased(VK_RBUTTON))
            {
                if (isMouseTrapped)
                {
                    inputManager.ShowMouse();
                    inputManager.ReleaseMouse();
                    isMouseTrapped = false;
                }
            }

            if (inputManager.IsKeyPressed(VK_SHIFT)) camSpeed *= 3.0f;
            if (inputManager.IsKeyReleased(VK_SHIFT)) camSpeed /= 3.0f;

            if (inputManager.IsKeyPressed(VK_ESCAPE))
            {
                PostQuitMessage(0);
                break;
            }

            if (!Tga::Application::GetInstance()->BeginFrame() || !graphicsEngine.BeginFrame())
            {
                break;
            }

            if (ImGui::Begin("Tutorial 22: Cubemap Prefiltering & PBR"))
            {
                if (ImGui::CollapsingHeader("Cubemap Sourcing", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::Button("Capture Scene at Camera Position", ImVec2(-1, 0)))
                    {
                        Vector3f capturePos = camera->GetTransform().GetPosition();
                        uint32_t captureRes = outputResOptions[outputResIndex];

                        RenderTarget captureTarget = RenderTarget::Create({ captureRes, captureRes }, DXGI_FORMAT_R16G16B16A16_FLOAT);
                        DepthBuffer captureDepth = DepthBuffer::Create({ captureRes, captureRes });

                        bool success = prefilterSystem.CaptureSceneToCubemap(
                            captureTarget,
                            [&](uint32_t faceIndex)
                            {
                                captureTarget.SetAsActiveTarget(&captureDepth);
                                captureTarget.Clear();
                                captureDepth.Clear();

                                GraphicsStateStack& stack = graphicsEngine.GetGraphicsStateStack();
                                Camera captureCam;
                                captureCam.SetTransform(CubemapPrefilter::GetCubemapCameraTransform(faceIndex, capturePos));
                                captureCam.SetPerspectiveProjection(90.0f, { static_cast<float>(captureRes), static_cast<float>(captureRes) }, 0.1f, 50000.0f);
                                stack.SetCamera(captureCam);

                                RenderScene(renderData, graphicsEngine);
                            },
                            baseCubemap
                        );

                        if (success)
                        {
                            prefilterSystem.GeneratePrefilteredCubemap(baseCubemap.srv.Get(), baseCubemap.size, outputResOptions[outputResIndex], sampleCountOptions[sampleCountIndex], prefilteredCubemap);
                            renderData.ambientLight->cubemap = prefilteredCubemap.resource.get();
                            statusMessage = "Scene captured and prefiltered successfully (" + std::to_string(captureRes) + "x" + std::to_string(captureRes) + ")!";
                            statusIsError = false;
                        }
                    }

                    if (ImGui::Button("Load Cubemap (.dds)...", ImVec2(-1, 0)))
                    {
                        std::string filePath;
                        if (FileDialogs::OpenFile(windowHandle, L"DirectDraw Surface (*.dds)\0*.dds\0All Files (*.*)\0*.*\0", L"Open Cubemap DDS", filePath))
                        {
                            if (prefilterSystem.LoadBaseFromDDS(filePath, baseCubemap))
                            {
                                prefilteredCubemap.Reset();
                                renderData.ambientLight->cubemap = baseCubemap.resource.get();
                                statusMessage = "Loaded DDS cubemap (" + std::to_string(baseCubemap.size) + "x" + std::to_string(baseCubemap.size) + ", " + std::to_string(baseCubemap.mipLevels) + " mips): " + filePath;
                                statusIsError = false;
                            }
                            else
                            {
                                statusMessage = "Failed to load DDS cubemap: " + filePath;
                                statusIsError = true;
                            }
                        }
                    }

                    if (ImGui::Button("Load Equirectangular Panorama (.hdr / .png / .dds / .tga)...", ImVec2(-1, 0)))
                    {
                        std::string filePath;
                        if (FileDialogs::OpenFile(windowHandle, L"Panorama Files (*.hdr;*.png;*.jpg;*.dds;*.tga)\0*.hdr;*.png;*.jpg;*.dds;*.tga\0Radiance HDR (*.hdr)\0*.hdr\0All Files (*.*)\0*.*\0", L"Open Panorama Image", filePath))
                        {
                            if (prefilterSystem.LoadBaseFromEquirectangular(filePath, outputResOptions[outputResIndex], baseCubemap))
                            {
                                prefilterSystem.GeneratePrefilteredCubemap(baseCubemap.srv.Get(), baseCubemap.size, outputResOptions[outputResIndex], sampleCountOptions[sampleCountIndex], prefilteredCubemap);
                                renderData.ambientLight->cubemap = prefilteredCubemap.resource.get();
                                statusMessage = "Panorama converted and prefiltered (" + std::to_string(outputResOptions[outputResIndex]) + "x" + std::to_string(outputResOptions[outputResIndex]) + "): " + filePath;
                                statusIsError = false;
                            }
                            else
                            {
                                statusMessage = "Failed to convert panorama: " + filePath;
                                statusIsError = true;
                            }
                        }
                    }

                    if (ImGui::Button("Load 4x3 Cube Cross (.png / .hdr / .dds / .jpg)...", ImVec2(-1, 0)))
                    {
                        std::string filePath;
                        if (FileDialogs::OpenFile(windowHandle, L"Image Files (*.png;*.hdr;*.jpg;*.dds;*.tga)\0*.png;*.hdr;*.jpg;*.dds;*.tga\0All Files (*.*)\0*.*\0", L"Open Cube Cross Image", filePath))
                        {
                            if (prefilterSystem.LoadBaseFromCubeCross(filePath, baseCubemap))
                            {
                                prefilterSystem.GeneratePrefilteredCubemap(baseCubemap.srv.Get(), baseCubemap.size, outputResOptions[outputResIndex], sampleCountOptions[sampleCountIndex], prefilteredCubemap);
                                renderData.ambientLight->cubemap = prefilteredCubemap.resource.get();
                                statusMessage = "Cube cross converted and prefiltered (" + std::to_string(outputResOptions[outputResIndex]) + "x" + std::to_string(outputResOptions[outputResIndex]) + "): " + filePath;
                                statusIsError = false;
                            }
                            else
                            {
                                statusMessage = "Failed to load Cube Cross image: " + filePath;
                                statusIsError = true;
                            }
                        }
                    }
                }

                if (ImGui::CollapsingHeader("Prefilter Pipeline", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Combo("Output Resolution", &outputResIndex, outputResNames, IM_ARRAYSIZE(outputResNames));
                    ImGui::Combo("Sample Count", &sampleCountIndex, sampleCountNames, IM_ARRAYSIZE(sampleCountNames));

                    if (ImGui::Button("Regenerate Prefiltered Cubemap", ImVec2(-1, 0)))
                    {
                        if (baseCubemap.IsValid())
                        {
                            prefilterSystem.GeneratePrefilteredCubemap(
                                baseCubemap.srv.Get(),
                                baseCubemap.size,
                                outputResOptions[outputResIndex],
                                sampleCountOptions[sampleCountIndex],
                                prefilteredCubemap
                            );
                            renderData.ambientLight->cubemap = prefilteredCubemap.resource.get();
                            statusMessage = "Prefiltered cubemap regenerated (" + std::to_string(outputResOptions[outputResIndex]) + "x" + std::to_string(outputResOptions[outputResIndex]) + ", " + std::to_string(sampleCountOptions[sampleCountIndex]) + " samples).";
                            statusIsError = false;
                        }
                        else
                        {
                            statusMessage = "No active base cubemap to prefilter.";
                            statusIsError = true;
                        }
                    }
                }

                if (ImGui::CollapsingHeader("Preview & Scene Controls", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Show Skybox Background", &renderData.showSkybox);
                    if (renderData.showSkybox)
                    {
                        int maxMip = prefilteredCubemap.IsValid() ? static_cast<int>(prefilteredCubemap.mipLevels - 1) : (baseCubemap.IsValid() ? static_cast<int>(baseCubemap.mipLevels - 1) : 10);
                        renderData.skyboxMipLevel = std::clamp(renderData.skyboxMipLevel, 0, maxMip);
                        ImGui::SliderInt("Skybox Preview Mip Level", &renderData.skyboxMipLevel, 0, maxMip, "Mip %d");
                    }

                    ImGui::SliderFloat("Environment & Skybox Intensity", &renderData.environmentIntensity, 0.0f, 5.0f, "%.2fx");
                    ImGui::Checkbox("Enable Ambient Light (IBL)", &renderData.enableAmbientLight);

                    ImGui::Separator();

                    {
                        ImGui::Checkbox("Enable Directional Light", &renderData.enableDirectionalLight);

                        bool lightChanged = false;
                        lightChanged |= ImGui::SliderFloat3("Light Rotation", dirLightEuler, -180.0f, 180.0f, "%.1f deg");
                        lightChanged |= ImGui::SliderFloat("Light Intensity", &dirLightIntensity, 0.0f, 10.0f, "%.2fx");
                        lightChanged |= ImGui::ColorEdit3("Light Color", dirLightColor);

                        if (lightChanged)
                        {
                            renderData.directionalLight->transform = Matrix4x4f::CreateFromRollPitchYaw(Rotator(dirLightEuler[0], dirLightEuler[1], dirLightEuler[2]));
                            renderData.directionalLight->color = Color::FromLinear(
                                dirLightColor[0] * dirLightIntensity,
                                dirLightColor[1] * dirLightIntensity,
                                dirLightColor[2] * dirLightIntensity
                            );
                        }
                    }

                    ImGui::Checkbox("Enable Tonemapping", &renderData.enableTonemapping);

                }

                if (ImGui::CollapsingHeader("DDS Cubemap Export", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ID3D11Texture2D* targetTex = prefilteredCubemap.IsValid() ? prefilteredCubemap.texture.Get() : baseCubemap.texture.Get();

                    if (ImGui::Button("Export to BC6H DDS (HDR Compressed)...", ImVec2(-1, 0)))
                    {
                        if (targetTex)
                        {
                            std::string savePath;
                            if (FileDialogs::SaveFile(windowHandle, L"DirectDraw Surface (*.dds)\0*.dds\0All Files (*.*)\0*.*\0", L"dds", L"prefiltered_cubemap_bc6h.dds", L"Export BC6H HDR DDS", savePath))
                            {
                                std::string errorMsg;
                                if (prefilterSystem.ExportToBC6HDDS(targetTex, savePath, errorMsg))
                                {
                                    statusMessage = "Exported BC6H HDR DDS successfully to: " + savePath;
                                    statusIsError = false;
                                }
                                else
                                {
                                    statusMessage = "Export failed: " + errorMsg;
                                    statusIsError = true;
                                }
                            }
                        }
                        else
                        {
                            statusMessage = "No cubemap texture available to export.";
                            statusIsError = true;
                        }
                    }

                    if (ImGui::Button("Export to Uncompressed Float DDS (R16F)...", ImVec2(-1, 0)))
                    {
                        if (targetTex)
                        {
                            std::string savePath;
                            if (FileDialogs::SaveFile(windowHandle, L"DirectDraw Surface (*.dds)\0*.dds\0All Files (*.*)\0*.*\0", L"dds", L"prefiltered_cubemap_r16f.dds", L"Export Uncompressed Float DDS", savePath))
                            {
                                std::string errorMsg;
                                if (prefilterSystem.ExportToFloatDDS(targetTex, savePath, errorMsg))
                                {
                                    statusMessage = "Exported Uncompressed Float DDS successfully to: " + savePath;
                                    statusIsError = false;
                                }
                                else
                                {
                                    statusMessage = "Export failed: " + errorMsg;
                                    statusIsError = true;
                                }
                            }
                        }
                        else
                        {
                            statusMessage = "No cubemap texture available to export.";
                            statusIsError = true;
                        }
                    }
                }

                // Status bar
                ImGui::Separator();
                ImVec4 statusColor = statusIsError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
                ImGui::TextColored(statusColor, "%s", statusMessage.c_str());

                if (prefilteredCubemap.IsValid())
                {
                    ImGui::Text("Active Cubemap (Prefiltered): %ux%u | %u Mips", prefilteredCubemap.size, prefilteredCubemap.size, prefilteredCubemap.mipLevels);
                }
                else if (baseCubemap.IsValid())
                {
                    ImGui::Text("Active Cubemap (Base DDS): %ux%u | %u Mips", baseCubemap.size, baseCubemap.size, baseCubemap.mipLevels);
                }
            }
            ImGui::End();

            // Render 3D Scene
            Render(renderData, *Tga::GraphicsEngine::GetInstance());

            graphicsEngine.EndFrame();
            Tga::Application::GetInstance()->EndFrame();
        }
    }

    Tga::GraphicsEngine::Shutdown();
    Tga::Application::Shutdown();
}
