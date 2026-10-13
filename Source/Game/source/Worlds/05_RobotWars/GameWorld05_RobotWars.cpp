#include "GameWorld05_RobotWars.h"
#include "../../Actors/Actor.h"
#include "../../Controllers/Steering/WanderController.h"
#include "../../Controllers/Steering/ObstacleAvoidanceController.h"
#include "../../Controllers/Steering/ArriveController.h"
#include "../../Controllers/DecisionMaking/DecisionTreeController.h"

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
    ConfigureDecisionTree();
    CreateRobots();
    myCombatController.SetEnemy(myActorManager.GetActor(1));
    myStateMachineCombatController.SetEnemy(myActorManager.GetActor(0));
    myBulletSpriteData.texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Sprites/ninjaStar.png");
    myRobotLabels[0].SetText("Decision Tree");
    myRobotLabels[1].SetText("State Machine");
    myControlsLabel.SetText("Space/Enter: damage robot 1/2 | R: reset battle");
    myControlsLabel.SetPosition({40.f, 24.f});
}

void GameWorld05_RobotWars::CreateRobots()
{
    const char* spritePaths[2] = {"Sprites/robot1.png", "Sprites/robot2.png"};
    for (std::size_t index = 0; index < myRobotLabels.size(); ++index)
    {
        Actor& robot = myActorManager.CreateActor(GetRobotSpawnPosition(index), spritePaths[index], nullptr);
        robot.SetMaxSpeed(180.f);
        robot.SetMaxForce(1200.f);
        robot.SetMass(1.f);
        robot.SetRadius(25.f);
        robot.EnableHealth(100.f, 40.f);
        robot.EnableRespawn(GetRobotSpawnPosition(index), 3.f);
        if (index == 0)
        {
            robot.SetController(std::make_unique<DecisionTreeController>(myRobotDecisionTree));
        }
        else
        {
            robot.SetController(std::make_unique<StateMachineController>(
                myStateMachineWanderController, myStateMachineArriveController, myStateMachineCombatController));
        }
        robot.AddController(std::make_unique<ObstacleAvoidanceController>(myWallAvoidanceSettings, myWallObstacles, myArenaBounds));
    }
}

void GameWorld05_RobotWars::Update(float aTimeDelta)
{
    UpdateTestInput();
    myCombatController.UpdateCooldown(aTimeDelta);
    myStateMachineCombatController.UpdateCooldown(aTimeDelta);
    myActorManager.Update(aTimeDelta);
    // Resolve bullet damage before healing, so a fatal hit cannot be undone.
    myBulletSystem.Update(aTimeDelta, *myArenaBounds);
    UpdateHealthPool(aTimeDelta);
}

void GameWorld05_RobotWars::Render()
{
    DrawHealthPool();
    myActorManager.Draw();
    DrawBullets();
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
        const char* woundedStatus = robot.IsAlive() && robot.IsWounded() ? " (wounded)" : "";
        const char* selectedActionName = "";
        if (!robot.IsAlive())
        {
            selectedActionName = " | Defeated";
        }
        else if (index == 0)
        {
            // Show the action actually used in this frame. Pool healing happens
            // after movement, so HP may already have crossed the threshold.
            const DecisionTreeController& treeController = static_cast<const DecisionTreeController&>(*robot.GetController());
            if (treeController.GetSelectedAction() == &myHealingAction)
                selectedActionName = " | Heal";
            else if (treeController.GetSelectedAction() == &myCombatAction)
                selectedActionName = " | Combat";
            else
                selectedActionName = " | Wander";
        }
        else
        {
            const StateMachineController& controller = static_cast<const StateMachineController&>(*robot.GetController());
            switch (controller.GetState())
            {
            case RobotState::Heal: selectedActionName = " | Heal"; break;
            case RobotState::Combat: selectedActionName = " | Combat"; break;
            case RobotState::Wander: selectedActionName = " | Wander"; break;
            }
        }
        char labelText[128] = {};
        std::snprintf(labelText, sizeof(labelText), "%s | HP %.0f/%.0f%s%s",
            robotNames[index], robot.GetHealth(), robot.GetMaxHealth(), woundedStatus, selectedActionName);
        if (!robot.IsAlive())
            std::snprintf(labelText, sizeof(labelText), "%s | HP 0/%.0f | Respawn %.1fs",
                robotNames[index], robot.GetMaxHealth(), robot.GetRespawnTimeRemaining());
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

void GameWorld05_RobotWars::ConfigureDecisionTree()
{
    WanderControllerData wanderData;
    wanderData.randomSeed = 1u;
    myDecisionTreeWanderController.SetWanderControllerData(wanderData);
    wanderData.randomSeed = 2u;
    myStateMachineWanderController.SetWanderControllerData(wanderData);

    // The healing leaf reuses Arrive with one fixed target at the pool.
    ArriveControllerData arriveData;
    arriveData.useRandomTargets = false;
    arriveData.slowingRadius = myHealthPool.radius;
    myDecisionTreeArriveController.SetArriveControllerData(arriveData);
    myDecisionTreeArriveController.SetTargetPosition(myHealthPool.position);
    myStateMachineArriveController.SetArriveControllerData(arriveData);
    myStateMachineArriveController.SetTargetPosition(myHealthPool.position);
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
    if (myInput->IsKeyPressed(Keys::R))
        ResetCombatTest();
    if (myInput->IsKeyPressed(Keys::SPACE))
        myActorManager.GetActor(0).TakeDamage(25.f);
    if (myInput->IsKeyPressed(Keys::RETURN))
        myActorManager.GetActor(1).TakeDamage(25.f);
}

CommonUtilities::Vector2f GameWorld05_RobotWars::GetRobotSpawnPosition(std::size_t aRobotIndex) const
{
    // Start within detection range so the combat test is visible immediately.
    const CommonUtilities::Vector2f arenaSize = myArenaBounds->GetMax() - myArenaBounds->GetMin();
    const float horizontalFraction = aRobotIndex == 0 ? 0.25f : 0.55f;
    const CommonUtilities::Vector2f spawnOffset = {arenaSize.x * horizontalFraction, arenaSize.y * 0.5f};
    return myArenaBounds->GetMin() + spawnOffset;
}

void GameWorld05_RobotWars::ResetCombatTest()
{
    // Clear flying shots before restoring their targets for another test.
    myBulletSystem.Clear();
    myCombatController.ResetCooldown();
    myStateMachineCombatController.ResetCooldown();
    static_cast<StateMachineController&>(*myActorManager.GetActor(1).GetController()).Reset();
    for (std::size_t robotIndex = 0; robotIndex < myActorManager.GetActorCount(); ++robotIndex)
    {
        Actor& robot = myActorManager.GetActor(robotIndex);
        robot.Stop();
        robot.EnableHealth(100.f, 40.f);
        robot.SetPosition(GetRobotSpawnPosition(robotIndex));
    }
}

void GameWorld05_RobotWars::DrawBullets()
{
    Tga::SpriteDrawer& drawer = Tga::GraphicsEngine::GetInstance()->GetSpriteDrawer();
    for (const Bullet& bullet : myBulletSystem.GetBullets())
    {
        Tga::Sprite2DInstanceData bulletSprite;
        bulletSprite.position = bullet.position.ToTga();
        bulletSprite.size = {bullet.radius * 2.f, bullet.radius * 2.f};
        bulletSprite.color = {1.f, 0.9f, 0.2f, 1.f};
        drawer.Draw(myBulletSpriteData, bulletSprite);
    }
}
