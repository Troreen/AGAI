#include "GameWorld05_RobotWars.h"
#include "../../Actors/Actor.h"
#include "../../Controllers/Steering/WanderController.h"
#include "../../Controllers/Steering/ObstacleAvoidanceController.h"

#include <tge/application.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/graphics/GraphicsEngine.h>

void GameWorld05_RobotWars::Init(const CommonUtilities::InputHandler&)
{
    const Tga::Vector2ui renderSize = Tga::Application::GetInstance()->GetRenderSize();
    const CommonUtilities::Vector2f resolution = {static_cast<float>(renderSize.x), static_cast<float>(renderSize.y)};
    myArenaBounds = std::make_shared<RectTraversalBounds>(resolution * 0.05f, resolution * 0.95f);

    CreateRobots();
    myRobotLabels[0].SetText("Decision Tree");
    myRobotLabels[1].SetText("State Machine");
}

void GameWorld05_RobotWars::CreateRobots()
{
    const CommonUtilities::Vector2f arenaSize = myArenaBounds->GetMax() - myArenaBounds->GetMin();
    const char* spritePaths[2] = {"Sprites/robot1.png", "Sprites/robot2.png"};
    for (std::size_t index = 0; index < myRobotLabels.size(); ++index)
    {
        const float horizontalFraction = index == 0 ? 0.25f : 0.75f;
        const CommonUtilities::Vector2f spawnOffset = {arenaSize.x * horizontalFraction, arenaSize.y * 0.5f};
        WanderControllerData wanderData;
        wanderData.randomSeed = static_cast<unsigned int>(index + 1);
        Actor& robot = myActorManager.CreateActor(myArenaBounds->GetMin() + spawnOffset, spritePaths[index], std::make_unique<WanderController>(wanderData));
        robot.SetMaxSpeed(180.f);
        robot.SetMaxForce(1200.f);
        robot.SetMass(1.f);
        robot.SetRadius(25.f);
        robot.AddController(std::make_unique<ObstacleAvoidanceController>(myWallAvoidanceSettings, myWallObstacles, myArenaBounds));
    }
}

void GameWorld05_RobotWars::Update(float aTimeDelta)
{
    myActorManager.Update(aTimeDelta);
}

void GameWorld05_RobotWars::Render()
{
    myActorManager.Draw();
    DrawLabels();
    DrawWallDebug();
}

void GameWorld05_RobotWars::DrawLabels()
{
    for (std::size_t index = 0; index < myRobotLabels.size(); ++index)
    {
        const Actor& robot = myActorManager.GetActor(index);
        Tga::Text& label = myRobotLabels[index];
        label.SetPosition((robot.GetPosition() + CommonUtilities::Vector2f(-label.GetWidth() * 0.5f, 45.f)).ToTga());
        label.Render();
    }
}

void GameWorld05_RobotWars::DrawWallDebug()
{
#ifndef _RETAIL
    Tga::DebugDrawer& drawer = Tga::GraphicsEngine::GetInstance()->GetDebugDrawer();
    const CommonUtilities::Vector2f minimum = myArenaBounds->GetMin();
    const CommonUtilities::Vector2f maximum = myArenaBounds->GetMax();
    const Tga::Vector2f bottomRight = {maximum.x, minimum.y};
    const Tga::Vector2f topLeft = {minimum.x, maximum.y};
    const Tga::Color wallColor = {0.8f, 0.3f, 1.f, 1.f};
    drawer.DrawLine(minimum.ToTga(), bottomRight, wallColor);
    drawer.DrawLine(bottomRight, maximum.ToTga(), wallColor);
    drawer.DrawLine(maximum.ToTga(), topLeft, wallColor);
    drawer.DrawLine(topLeft, minimum.ToTga(), wallColor);
#endif
}
