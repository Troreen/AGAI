#include "GameWorld.h"

#include <tge/application.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/model/ModelFactory.h>
#include <tge/texture/TextureManager.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/scene/Scene.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <tge/settings/settings.h>
#include <tge/input/InputManager.h>
#include <tge/text/TextService.h>

#include <cmath>

// 2048x2048 shadow map provides crisp shadows across the camera view
constexpr float ShadowMapResolution = 2048.0f;

GameWorld::GameWorld()
	: myDirectionalLightRotation(30.0f, 45.0f, 0.0f) // initial sun angle (pitch, yaw, roll)
	, myPlayerPosition(0.0f, 0.0f, 0.0f)
{
}

GameWorld::~GameWorld()
{
}

void GameWorld::Init()
{
	// Load all .tgo object definitions so the game knows models and materials for placed objects
	mySceneObjectDefinitionManager.Init(Tga::Settings::GameAssetRoot().c_str());

	Tga::Vector2ui resolution = Tga::Application::GetInstance()->GetRenderSize();

	myCamera.SetPerspectiveProjection(
		45.0f,
		{ (float)resolution.x, (float)resolution.y },
		500.0f,
		10000.0f
	);

	// Multi-pass render buffers:
	// - Shadow map: depth buffer captured from the sun's perspective
	// - Z-Prepass: screen-depth buffer rendered from main camera, used as input for SSAO
	// - SSAO render target: stores calculated ambient occlusion brightness
	myDirectionalShadowMap = Tga::DepthBuffer::Create({ (unsigned int)ShadowMapResolution, (unsigned int)ShadowMapResolution });
	myZPrepassDepthBuffer = Tga::DepthBuffer::Create(resolution);
	mySsaoRenderTarget = Tga::RenderTarget::Create(resolution, DXGI_FORMAT_R8G8B8A8_UNORM);

	// Custom shaders for volume SSAO and Lambert shading with shadow & AO support
	mySsaoEffect.Init("shaders/VolumeSsaoPS");
	myLambertShadowSsaoShader.Init("shaders/model_shader_VS", "shaders/LambertWithAOandShadowsPS");

	Tga::Font font = Tga::GraphicsEngine::GetInstance()->GetTextService().GetOrLoad("Text/arial.ttf", Tga::FontSize_18);
	myExplanationText = Tga::Text(font);
	myExplanationText.SetText("WASD: Move Player | Q/E: Rotate Directional Light | TAB: Toggle Shadows & Volume SSAO");
}

void GameWorld::LoadScene(Tga::Scene& aScene)
{
	mySceneInstances.clear();

	Tga::TextureManager& textureManager = Tga::GraphicsEngine::GetInstance()->GetTextureManager();
	std::vector<Tga::ScenePropertyDefinition> sceneObjectProperties;

	// Iterate through objects placed in the GoEditor level scene (.leveldata)
	for (const auto& p : aScene.GetSceneObjects())
	{
		sceneObjectProperties.clear();
		// Combine archetype TGO properties with level-specific overrides
		p.second->CalculateCombinedPropertySet(mySceneObjectDefinitionManager, sceneObjectProperties);

		// An object named "PlayerSpawn" in the editor determines where the player starts
		bool isPlayerSpawn = (Tga::StringRegistry().RegisterOrGetString(p.second->GetName()) == "PlayerSpawn"_tgaid);
		if (isPlayerSpawn)
		{
			myPlayerPosition = p.second->GetTransform().GetPosition();
		}

		for (Tga::ScenePropertyDefinition& property : sceneObjectProperties)
		{
			if (property.type == Tga::GetPropertyType<Tga::CopyOnWriteWrapper<Tga::SceneModel>>())
			{
				const Tga::SceneModel& value = property.value.Get<Tga::CopyOnWriteWrapper<Tga::SceneModel>>()->Get();

				Tga::StringId path = value.path;
				Tga::FilePathStream dummyPath;
				if (path.IsEmpty() || !Tga::Settings::ResolveAssetPath(path, dummyPath))
					continue;

				// Load model and set up textures (slot 0 diffuse is sRGB, data textures linear)
				if (Tga::ModelFactory::GetInstance().GetModel(path.GetString()))
				{
					Tga::ModelInstance instance = Tga::ModelFactory::GetInstance().GetModelInstance(path.GetString());
					int meshCount = (int)instance.GetModel()->GetMeshCount();
					if (meshCount > MAX_MESHES_PER_MODEL)
						meshCount = MAX_MESHES_PER_MODEL;

					for (int i = 0; i < meshCount; i++)
					{
						for (int j = 0; j < 4; j++)
						{
							if (!value.textures[i][j].IsEmpty())
							{
								Tga::TextureSrgbMode srgbMode = (j == 0) ? Tga::TextureSrgbMode::ForceSrgbFormat : Tga::TextureSrgbMode::ForceNoSrgbFormat;
								Tga::Texture* texture = textureManager.GetTexture(value.textures[i][j].GetString(), srgbMode);
								if (texture != nullptr)
									instance.SetTexture(i, j, texture);
							}
						}
					}

					if (isPlayerSpawn)
					{
						myPlayerInstance = instance;
						myHasPlayerInstance = true;
					}
					else
					{
						mySceneInstances.push_back({ p.second->GetTransform(), instance });
					}
				}
			}
		}
	}
}

void GameWorld::Update(float aTimeDelta, Tga::InputManager& inputManager)
{
	// Top-down / Isometric movement math:
	// The camera is angled at -45 deg yaw. To make 'W' move straight up on screen,
	// 'S' down, 'A' left, and 'D' right, movement is mapped along diagonal world vectors:
	Tga::Vector3f forward(-1.0f, 0.0f, 1.0f);
	Tga::Vector3f right(1.0f, 0.0f, 1.0f);
	Tga::Vector3f movement(0.0f);

	if (inputManager.IsKeyHeld('W'))
	{
		movement += forward * 1.0f;
	}
	if (inputManager.IsKeyHeld('S'))
	{
		movement += forward * -1.0f;
	}
	if (inputManager.IsKeyHeld('A'))
	{
		movement += right * -1.0f;
	}
	if (inputManager.IsKeyHeld('D'))
	{
		movement += right * 1.0f;
	}

	if (movement.LengthSqr() > 0.1f)
	{
		movement.Normalize();
		constexpr float PlayerSpeed = 250.0f;
		myPlayerPosition += movement * aTimeDelta * PlayerSpeed;
	}

	// Rotate the sun (directional light) with Q/E to test dynamic shadows in real time
	if (inputManager.IsKeyHeld('Q'))
	{
		myDirectionalLightRotation.Y += 30.0f * aTimeDelta;
	}
	if (inputManager.IsKeyHeld('E'))
	{
		myDirectionalLightRotation.Y -= 30.0f * aTimeDelta;
	}

	// Toggle shadows and SSAO with TAB to compare lighting with and without post/shadow passes
	if (inputManager.IsKeyPressed(VK_TAB))
	{
		myUseShadowsAndAO = !myUseShadowsAndAO;
	}
}

void GameWorld::DrawOpaqueObjects(bool aIsZPrepass)
{
	Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();
	Tga::ModelDrawer& modelDrawer = graphicsEngine.GetModelDrawer();
	Tga::GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();

	auto drawModel = [&](const Tga::ModelInstance& model)
	{
		if (!aIsZPrepass)
		{
			// the final pass we use our custom ssao shader
			modelDrawer.Draw(model, myLambertShadowSsaoShader);
		}
		else
		{
			// for other models we use regular lambert. This is a bit overkill since we don't use the color
			// but we need to run pixel shader for meshes with transparency to correctly discard
			modelDrawer.DrawLambert(model);
		}
	};

	for (const auto& item : mySceneInstances)
	{
		graphicsStateStack.Push();
		graphicsStateStack.SetTransform(item.transform);
		drawModel(item.instance);
		graphicsStateStack.Pop();
	}

	if (myHasPlayerInstance)
	{
		graphicsStateStack.Push();
		Tga::Matrix4x4f playerTransform = Tga::Matrix4x4f::CreateFromScale(0.5f);
		playerTransform.SetPosition(myPlayerPosition + Tga::Vector3f{ 0.0f, 50.0f, 0.0f });
		graphicsStateStack.SetTransform(playerTransform);
		drawModel(myPlayerInstance);
		graphicsStateStack.Pop();
	}
}

void GameWorld::Render()
{
	Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();
	Tga::GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();
	Tga::Vector2ui resolution = Tga::Application::GetInstance()->GetRenderSize();

	// -------------------------------------------------------------------------
	// Pass 1: Directional Shadow Map Pass
	// Directional light uses an orthographic camera oriented towards the sun
	// -------------------------------------------------------------------------
	Tga::Camera shadowCamera;
	shadowCamera.GetTransform().SetRotation(myDirectionalLightRotation);

	float shadowMapScale = 1500.0f;
	Tga::Vector3f shadowPos;

	{
		// Texel snapping (stabilization):
		// As the player moves, the shadow camera moves with them. Snapping its position
		// to exact world-space multiples of a shadow map texel prevents shadow edges
		// from shimmering / crawling as the camera travels.
		Tga::Matrix4x4f shadowCameraMatrix = shadowCamera.GetTransform();
		Tga::Vector3f forward = shadowCameraMatrix.GetForward();
		Tga::Vector3f right = shadowCameraMatrix.GetRight();
		Tga::Vector3f up = shadowCameraMatrix.GetUp();

		float f = myPlayerPosition.Dot(forward);
		float r = myPlayerPosition.Dot(right);
		float u = myPlayerPosition.Dot(up);

		float texelWorldSize = (2.0f * shadowMapScale / ShadowMapResolution);
		r = texelWorldSize * std::round(r / texelWorldSize);
		u = texelWorldSize * std::round(u / texelWorldSize);

		shadowPos = f * forward + r * right + u * up;
	}

	shadowCamera.GetTransform().SetPosition(shadowPos);
	shadowCamera.SetOrtographicProjection(-shadowMapScale, shadowMapScale, -shadowMapScale, shadowMapScale, -5000.0f, 5000.0f);

	if (myUseShadowsAndAO)
	{
		graphicsStateStack.Push();
		graphicsStateStack.SetCamera(shadowCamera);
		graphicsStateStack.SetBlendState(Tga::BlendState::Disabled);

		// Unbind shadow map from PS to avoid D3D11 resource hazards
		ID3D11ShaderResourceView* nullView = nullptr;
		Tga::DX11::Context->PSSetShaderResources(8, 1, &nullView);

		myDirectionalShadowMap.Clear();
		myDirectionalShadowMap.SetAsActiveTarget();

		DrawOpaqueObjects(true);
		graphicsStateStack.Pop();
	}

	// -------------------------------------------------------------------------
	// Pass 2: Z-Prepass (Depth Prepass)
	// Renders scene depth from the main camera view, needed as input for SSAO
	// -------------------------------------------------------------------------
	Tga::Camera mainCamera = myCamera;
	mainCamera.SetPerspectiveProjection(
		45.0f,
		{ (float)resolution.x, (float)resolution.y },
		500.0f,
		10000.0f
	);
	mainCamera.GetTransform().SetRotation(Tga::Rotator(30.0f, -45.0f, 0.0f)); // 30 deg pitch, -45 deg yaw
	mainCamera.GetTransform().SetPosition(myPlayerPosition - 1500.0f * mainCamera.GetTransform().GetForward());

	if (myUseShadowsAndAO)
	{
		graphicsStateStack.Push();
		graphicsStateStack.SetCamera(mainCamera);
		graphicsStateStack.SetBlendState(Tga::BlendState::Disabled);

		myZPrepassDepthBuffer.Clear(1.0f);
		myZPrepassDepthBuffer.SetAsActiveTarget();

		DrawOpaqueObjects(true);
		graphicsStateStack.Pop();
	}

	// -------------------------------------------------------------------------
	// Pass 3: Volume SSAO Pass
	// Fullscreen shader calculates contact occlusion from the Z-prepass depth buffer
	// -------------------------------------------------------------------------
	if (myUseShadowsAndAO)
	{
		graphicsStateStack.Push();
		graphicsStateStack.SetCamera(mainCamera);

		mySsaoRenderTarget.Clear({ 1.0f, 1.0f, 1.0f, 1.0f });
		mySsaoRenderTarget.SetAsActiveTarget();

		// Bind Z-prepass depth buffer on slot 1 for the SSAO pixel shader
		myZPrepassDepthBuffer.SetAsResourceOnSlot(1);
		mySsaoEffect.Render();

		ID3D11ShaderResourceView* nullView = nullptr;
		Tga::DX11::Context->PSSetShaderResources(1, 1, &nullView);

		graphicsStateStack.Pop();
	}

	// -------------------------------------------------------------------------
	// Pass 4: Forward Lighting Pass
	// Draws models with diffuse lighting, sampling shadow map (slot 8) & SSAO (slot 9)
	// -------------------------------------------------------------------------
	{
		graphicsStateStack.Push();

		Tga::DX11::BackBuffer->SetAsActiveTarget(Tga::DX11::DepthBuffer);
		graphicsStateStack.SetBlendState(Tga::BlendState::Disabled);
		graphicsStateStack.SetCamera(mainCamera);

		// Directional light matrix (invProj * lightTransform) transforms world positions to shadow map UVs
		Tga::DirectionalLight directionalLight(
			Tga::Matrix4x4f::Inverse(shadowCamera.GetProjection()) * shadowCamera.GetTransform(),
			{ 1.2f, 1.05f, 0.8f },
			0.0f
		);
		graphicsStateStack.SetDirectionalLight(directionalLight);

		Tga::AmbientLight ambientLight;
		ambientLight.color = { 0.1f, 0.3f, 0.4f };
		graphicsStateStack.SetAmbientLight(ambientLight);

		// Custom parameter .x controls shader toggling (1.0 = shadows & SSAO on, 0.0 = off)
		graphicsStateStack.SetCustomShaderParameters(Tga::Vector4f(myUseShadowsAndAO ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f));

		if (myUseShadowsAndAO)
		{
			myDirectionalShadowMap.SetAsResourceOnSlot(8);
			mySsaoRenderTarget.SetAsResourceOnSlot(9);
		}

		DrawOpaqueObjects(false);

		ID3D11ShaderResourceView* nullView = nullptr;
		Tga::DX11::Context->PSSetShaderResources(8, 1, &nullView);
		Tga::DX11::Context->PSSetShaderResources(9, 1, &nullView);
		graphicsStateStack.Pop();
	}

	// -------------------------------------------------------------------------
	// Pass 5: 2D UI Overlay (rendered to backbuffer without depth testing)
	// -------------------------------------------------------------------------
	Tga::DX11::BackBuffer->SetAsActiveTarget();

	Tga::Vector2f floatResolution = { (float)resolution.x, (float)resolution.y };
	myExplanationText.SetPosition(Tga::Vector2f{ 0.02f, 0.02f } * floatResolution);
	myExplanationText.Render();
}



