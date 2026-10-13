#include "GameWorld05_RobotWars.h"
#include "../../Actors/Actor.h"
#include "../../Controllers/Steering/WanderController.h"
#include "../../Controllers/Steering/ObstacleAvoidanceController.h"
#include "../../Controllers/Steering/ArriveController.h"

#include <tge/application.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/graphics/GraphicsEngine.h>
#include <Input/InputHandler.h>
#include <cstdio>

void GameWorld05_RobotWars::Init(const CommonUtilities::InputHandler& aInput)
{
    myInput = &aInput;
    const Tga::Vector2ui renderSize = Tga::Application::GetInstance()->GetRenderSize();
    const CommonUtilities::Vector2f resolution = {static_cast<float>(renderSize.x), static_cast<float>(renderSize.y)};
    myArenaBounds = std::make_shared<RectTraversalBounds>(resolution * 0.05f, resolution * 0.95f);

    CreateHealthPool();
    CreateRobots();
    myRobotLabels[0].SetText("Decision Tree");
    myRobotLabels[1].SetText("State Machine");
    myControlsLabel.SetText("Space: damage robot 1 | Enter: damage robot 2 | H: toggle robot 1 healing test");
    myControlsLabel.SetPosition({40.f, 24.f});
}

void GameWorld05_RobotWars::CreateRobots()
{
    const CommonUtilities::Vector2f arenaSize = myArenaBounds->GetMax() - myArenaBounds->GetMin();
    const char* spritePaths[2] = {"Sprites/robot1.png", "Sprites/robot2.png"};
    for (std::size_t index = 0; index < myRobotLabels.size(); ++index)
    {
        const float horizontalFraction = index == 0 ? 0.25f : 0.75f;
        const CommonUtilities::Vector2f spawnOffset = {arenaSize.x * horizontalFraction, arenaSize.y * 0.5f};

        Actor& robot = myActorManager.CreateActor(myArenaBounds->GetMin() + spawnOffset, spritePaths[index], nullptr);
        robot.SetMaxSpeed(180.f);
        robot.SetMaxForce(1200.f);
        robot.SetMass(1.f);
        robot.SetRadius(25.f);
        robot.EnableHealth(100.f, 40.f);
        SetRobotMovement(robot, index, false);
    }
}

void GameWorld05_RobotWars::Update(float aTimeDelta)
{
    UpdateTestInput();
    myActorManager.Update(aTimeDelta);
    UpdateHealthPool(aTimeDelta);
}

void GameWorld05_RobotWars::Render()
{
    DrawHealthPool();
    myActorManager.Draw();
    DrawLabels();
    DrawWallDebug();
}

void GameWorld05_RobotWars::DrawLabels()
{
    const char* robotNames[2] = {"Decision Tree", "State Machine"};
    for (std::size_t index = 0; index < myRobotLabels.size(); ++index)
    {
        const Actor& robot = myActorManager.GetActor(index);
        Tga::Text& label = myRobotLabels[index];
        char labelText[128] = {};
        std::snprintf(labelText, sizeof(labelText), "%s | HP %.0f/%.0f%s", robotNames[index], robot.GetHealth(), robot.GetMaxHealth(), robot.IsWounded() ? " (wounded)" : "");
        label.SetText(labelText);
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

void GameWorld05_RobotWars::CreateHealthPool()
{
    myHealthPool.position = (myArenaBounds->GetMin() + myArenaBounds->GetMax()) * 0.5f;
    myHealthPoolSpriteData.texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Sprites/healthPool.png");
    myHealthPoolSprite.position = myHealthPool.position.ToTga();
    myHealthPoolSprite.size = {myHealthPool.radius * 2.f, myHealthPool.radius * 2.f};
    myHealthPoolLabel.SetText("Health pool");
    myHealthPoolLabel.SetPosition((myHealthPool.position + CommonUtilities::Vector2f(-45.f, -myHealthPool.radius - 20.f)).ToTga());
}

void GameWorld05_RobotWars::SetRobotMovement(Actor& aRobot, std::size_t aRobotIndex, bool aSeekHealthPool)
{
    // Temporary manual movement selection for testing the health pool.
    if (aSeekHealthPool)
    {
        ArriveControllerData arriveData;
        arriveData.useRandomTargets = false;
        arriveData.slowingRadius = myHealthPool.radius;
        std::unique_ptr<ArriveController> arriveController = std::make_unique<ArriveController>(arriveData);
        arriveController->SetTargetPosition(myHealthPool.position);
        aRobot.SetController(std::move(arriveController));
    }
    else
    {
        WanderControllerData wanderData;
        wanderData.randomSeed = static_cast<unsigned int>(aRobotIndex + 1);
        aRobot.SetController(std::make_unique<WanderController>(wanderData));
    }
    aRobot.AddController(std::make_unique<ObstacleAvoidanceController>(myWallAvoidanceSettings, myWallObstacles, myArenaBounds));
}

void GameWorld05_RobotWars::UpdateHealthPool(float aDeltaTime)
{
    const float healingRadiusSquared = myHealthPool.radius * myHealthPool.radius;
    for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
    {
        Actor& robot = myActorManager.GetActor(index);
        if (robot.GetPosition().DistanceSqr(myHealthPool.position) <= healingRadiusSquared)
        {
            robot.Heal(myHealthPool.healingPerSecond * aDeltaTime);
        }
    }
}

void GameWorld05_RobotWars::DrawHealthPool()
{
    Tga::SpriteDrawer& drawer = Tga::GraphicsEngine::GetInstance()->GetSpriteDrawer();
    drawer.Draw(myHealthPoolSpriteData, myHealthPoolSprite);
    myHealthPoolLabel.Render();
    myControlsLabel.Render();
}

void GameWorld05_RobotWars::UpdateTestInput()
{
    if (myInput->IsKeyPressed(Keys::SPACE))
        myActorManager.GetActor(0).TakeDamage(25.f);
    if (myInput->IsKeyPressed(Keys::RETURN))
        myActorManager.GetActor(1).TakeDamage(25.f);
    if (myInput->IsKeyPressed(Keys::H))
    {
        myTestHealingMode = !myTestHealingMode;
        SetRobotMovement(myActorManager.GetActor(0), 0, myTestHealingMode);
    }
}
