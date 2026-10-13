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
    // Use the actual window size so the walls, pool, and spawn positions agree.
    myInput = &aInput;
    const Tga::Vector2ui renderSize = Tga::Application::GetInstance()->GetRenderSize();
    const CommonUtilities::Vector2f resolution = {static_cast<float>(renderSize.x), static_cast<float>(renderSize.y)};
    myArenaBounds = std::make_shared<RectTraversalBounds>(resolution * 0.05f, resolution * 0.95f);

    CreateHealthPool();
    ConfigureRobotMovement();
    CreateRobots();
    // Both actors must exist before either combat controller can borrow its enemy.
    myDecisionTreeCombatController.SetEnemy(myActorManager.GetActor(1));
    myStateMachineCombatController.SetEnemy(myActorManager.GetActor(0));
    myBulletSpriteData.texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Sprites/ninjaStar.png");
    myRobotLabels[0].SetText("Decision Tree");
    myRobotLabels[1].SetText("State Machine");
    myControlsLabel.SetText("Space/Enter: damage robot 1/2 | R: reset battle");
    myControlsLabel.SetPosition({40.f, 24.f});
}

void GameWorld05_RobotWars::CreateRobots()
{
    // Give both robots the same movement and health settings. Only their way
    // of choosing actions differs: robot 1 has a tree, robot 2 has states.
    const char* spritePaths[2] = {"Sprites/robot1.png", "Sprites/robot2.png"};
    for (std::size_t robotIndex = 0; robotIndex < myRobotLabels.size(); ++robotIndex)
    {
        Actor& robot = myActorManager.CreateActor(GetRobotSpawnPosition(robotIndex), spritePaths[robotIndex], nullptr);
        robot.SetMaxSpeed(180.f);
        robot.SetMaxForce(1200.f);
        robot.SetMass(1.f);
        robot.SetRadius(25.f);
        robot.EnableHealth(100.f, 40.f);
        robot.EnableRespawn(GetRobotSpawnPosition(robotIndex), 3.f);
        if (robotIndex == 0)
        {
            robot.SetController(std::make_unique<DecisionTreeController>(myDecisionTree));
        }
        else
        {
            robot.SetController(std::make_unique<StateMachineController>(
                myStateMachineWanderController, myStateMachineHealingController, myStateMachineCombatController));
        }
        // Wall avoidance adds steering alongside whichever action the AI chose.
        robot.AddController(std::make_unique<ObstacleAvoidanceController>(myWallAvoidanceSettings, myWallObstacles, myArenaBounds));
    }
}

void GameWorld05_RobotWars::Update(float aTimeDelta)
{
    UpdateBattleInput();
    // Shot timers keep counting down while a robot heals, wanders, or is dead.
    myDecisionTreeCombatController.UpdateCooldown(aTimeDelta);
    myStateMachineCombatController.UpdateCooldown(aTimeDelta);
    myActorManager.Update(aTimeDelta);
    // Resolve bullet damage before healing, so a fatal hit cannot be undone.
    myBulletSystem.Update(aTimeDelta, *myArenaBounds);
    UpdateHealthPool(aTimeDelta);
}

void GameWorld05_RobotWars::Render()
{
    // Draw the pool underneath the robots, then bullets and readable labels on top.
    DrawHealthPool();
    myActorManager.Draw();
    DrawBullets();
    DrawLabels();
    DrawWallDebug();
}

void GameWorld05_RobotWars::DrawLabels()
{
    const char* robotNames[2] = {"Decision Tree", "State Machine"};
    for (std::size_t robotIndex = 0; robotIndex < myRobotLabels.size(); ++robotIndex)
    {
        const Actor& robot = myActorManager.GetActor(robotIndex);
        Tga::Text& label = myRobotLabels[robotIndex];
        const char* woundedStatus = robot.IsAlive() && robot.IsWounded() ? " (wounded)" : "";
        const char* selectedBehaviourText = "";
        if (!robot.IsAlive())
        {
            selectedBehaviourText = " | Defeated";
        }
        else if (robotIndex == 0)
        {
            // Show the action actually used in this frame. Pool healing happens
            // after movement, so HP may already have crossed the threshold.
            const DecisionTreeController& treeController = static_cast<const DecisionTreeController&>(*robot.GetController());
            if (treeController.GetSelectedAction() == &myDecisionTreeHealingAction)
                selectedBehaviourText = " | Heal";
            else if (treeController.GetSelectedAction() == &myDecisionTreeCombatAction)
                selectedBehaviourText = " | Combat";
            else
                selectedBehaviourText = " | Wander";
        }
        else
        {
            const StateMachineController& stateMachineController = static_cast<const StateMachineController&>(*robot.GetController());
            switch (stateMachineController.GetState())
            {
            case RobotState::Heal: selectedBehaviourText = " | Heal"; break;
            case RobotState::Combat: selectedBehaviourText = " | Combat"; break;
            case RobotState::Wander: selectedBehaviourText = " | Wander"; break;
            }
        }
        char labelText[128] = {};
        std::snprintf(labelText, sizeof(labelText), "%s | HP %.0f/%.0f%s%s",
            robotNames[robotIndex], robot.GetHealth(), robot.GetMaxHealth(), woundedStatus, selectedBehaviourText);
        if (!robot.IsAlive())
            std::snprintf(labelText, sizeof(labelText), "%s | HP 0/%.0f | Respawn %.1fs",
                robotNames[robotIndex], robot.GetMaxHealth(), robot.GetRespawnTimeRemaining());
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

void GameWorld05_RobotWars::ConfigureRobotMovement()
{
    // Different seeds let the robots wander independently without changing speed.
    WanderControllerData wanderData;
    wanderData.randomSeed = 1u;
    myDecisionTreeWanderController.SetWanderControllerData(wanderData);
    wanderData.randomSeed = 2u;
    myStateMachineWanderController.SetWanderControllerData(wanderData);

    // Both healing actions use Arrive with the pool as a fixed target. Wandering
    // chooses new directions, but healing must keep aiming at the same place.
    ArriveControllerData arriveData;
    arriveData.useRandomTargets = false;
    arriveData.slowingRadius = myHealthPool.radius;
    myDecisionTreeHealingController.SetArriveControllerData(arriveData);
    myDecisionTreeHealingController.SetTargetPosition(myHealthPool.position);
    myStateMachineHealingController.SetArriveControllerData(arriveData);
    myStateMachineHealingController.SetTargetPosition(myHealthPool.position);
}

void GameWorld05_RobotWars::UpdateHealthPool(float aDeltaTime)
{
    // Healing belongs to the pool, not to an AI action. Any living robot inside
    // this radius recovers HP, even if it is currently wandering or fighting.
    const float healingRadiusSquared = myHealthPool.radius * myHealthPool.radius;
    for (std::size_t robotIndex = 0; robotIndex < myActorManager.GetActorCount(); ++robotIndex)
    {
        Actor& robot = myActorManager.GetActor(robotIndex);
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

void GameWorld05_RobotWars::UpdateBattleInput()
{
    // Pressed checks apply damage once per press, rather than every held frame.
    if (myInput->IsKeyPressed(Keys::R))
        ResetBattle();
    if (myInput->IsKeyPressed(Keys::SPACE))
        myActorManager.GetActor(0).TakeDamage(25.f);
    if (myInput->IsKeyPressed(Keys::RETURN))
        myActorManager.GetActor(1).TakeDamage(25.f);
}

CommonUtilities::Vector2f GameWorld05_RobotWars::GetRobotSpawnPosition(std::size_t aRobotIndex) const
{
    // Start within detection range so the battle begins without waiting for wandering.
    const CommonUtilities::Vector2f arenaSize = myArenaBounds->GetMax() - myArenaBounds->GetMin();
    const float horizontalFraction = aRobotIndex == 0 ? 0.25f : 0.55f;
    const CommonUtilities::Vector2f spawnOffset = {arenaSize.x * horizontalFraction, arenaSize.y * 0.5f};
    return myArenaBounds->GetMin() + spawnOffset;
}

void GameWorld05_RobotWars::ResetBattle()
{
    // Clear flying shots first so the fresh battle cannot inherit old damage.
    myBulletSystem.Clear();
    myDecisionTreeCombatController.ResetCooldown();
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
    // BulletSystem owns the gameplay positions; these sprites only display them.
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
