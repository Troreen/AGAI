#include "GameWorld.h"
#include "Actor.h"
#include "Boid3D.h"
#include "TraversalBounds.h"
#include <imgui/imgui.h>
#include <random>
#include <tge/application.h>
#include <tge/graphics/Camera.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/model/ModelFactory.h>
#include <tge/model/ModelInstance.h>
#include <tge/log/Log.h>

GameWorld::GameWorld() = default;
GameWorld::~GameWorld() = default;

void GameWorld::Init()
{
    myTraversalBounds3D = std::make_unique<RectTraversalBounds3D>(Tga::Vector3f(-1800.f, -500.f, -1800.f), Tga::Vector3f(1800.f, 1100.f, 1800.f));
    const auto size = Tga::Application::GetInstance()->GetRenderSize();
    myCamera = std::make_shared<Tga::Camera>();
    myCamera->SetPerspectiveProjection(75.f, { static_cast<float>(size.x), static_cast<float>(size.y) }, 0.1f, 10000.f);
    myCamera->GetTransform().SetPosition({ 0.f, 2700.f, -3600.f });
    myCamera->GetTransform().SetRotation(Tga::Rotator(32.f, 0.f, 0.f));
    myInputManager = std::make_unique<Tga::InputManager>(*Tga::Application::GetInstance()->GetHWND());
    myCameraController.Init(*myInputManager, *myCamera);
    myCameraController.SetMoveSpeed(2000.f);

    Tga::ModelFactory& factory = Tga::ModelFactory::GetInstance();
    myGround = std::make_shared<Tga::ModelInstance>(factory.GetModelInstance("Plane"));
    myGround->GetTransform().SetPosition({ 0.f, -500.f, 0.f });
    myGround->GetTransform().Scale({ 35.f, 1.f, 35.f });
    const std::vector<Tga::Vector3f> obstaclePositions = {{-450.f, 0.f, 150.f}, {500.f, 250.f, 650.f}, {350.f, -150.f, -650.f}};
    for (const Tga::Vector3f& position : obstaclePositions)
    {
        myObstacles.push_back({ position, 230.f });
        auto cube = std::make_shared<Tga::ModelInstance>(factory.GetModelInstance("Cube"));
        cube->GetTransform().SetPosition(position); cube->GetTransform().Scale({ 5.f, 5.f, 5.f });
        myObstacleModels.push_back(cube);
    }
    std::mt19937 randomGenerator(7u);
    std::uniform_real_distribution<float> randomX(-1500.f, 1500.f), randomY(-300.f, 900.f), randomZ(-1500.f, 1500.f);
    Tga::ModelInstance cone = factory.GetModelInstance("models/cone.fbx");
    if (!cone.IsValid())
    {
        ERROR_PRINT("Could not load boid model: models/cone.fbx");
        return;
    }
    for (int i = 0; i < 180; ++i)
        myBoids3D.push_back(std::make_unique<Boid3D>(cone, Tga::Vector3f(randomX(randomGenerator), randomY(randomGenerator), randomZ(randomGenerator)), i + 1));
}

void GameWorld::Update(float aTimeDelta)
{
    myInputManager->Update();
    myCameraController.Update(aTimeDelta);
    std::vector<Boid3D*> flock; flock.reserve(myBoids3D.size());
    for (auto& boid : myBoids3D) flock.push_back(boid.get());
    for (auto& boid : myBoids3D) boid->Update(aTimeDelta, flock, myObstacles, *myTraversalBounds3D);
}

void GameWorld::Render()
{
    if (ImGui::Begin("3D Boid Settings"))
    {
        if (!myBoids3D.empty())
        {
            Boid3DSettings settings = myBoids3D.front()->GetSettings();
            ImGui::SliderFloat("Perception Radius", &settings.perceptionRadius, 20.f, 1000.f);
            ImGui::SliderFloat("Obstacle Look Ahead", &settings.avoidanceLookAhead, 100.f, 1500.f);
            ImGui::SliderFloat("Alignment", &settings.alignmentWeight, 0.f, 5.f);
            ImGui::SliderFloat("Cohesion", &settings.cohesionWeight, 0.f, 5.f);
            ImGui::SliderFloat("Separation", &settings.separationWeight, 0.f, 5.f);
            for (auto& boid : myBoids3D) boid->SetSettings(settings);
        }
    }
    ImGui::End();
    auto& engine = *Tga::GraphicsEngine::GetInstance(); auto& state = engine.GetGraphicsStateStack();
    state.Push(); state.SetCamera(*myCamera); Tga::DX11::BackBuffer->SetAsActiveTarget(Tga::DX11::DepthBuffer);
    engine.GetModelDrawer().Draw(*myGround);
    for (const auto& obstacle : myObstacleModels) engine.GetModelDrawer().Draw(*obstacle);
    for (const auto& boid : myBoids3D) boid->Draw();
    state.Pop();
}
