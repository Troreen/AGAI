#include "GameWorld05_RobotWars.h"
#include "../../Actors/Actor.h"
#include "../../Controllers/Steering/WanderController.h"
#include "../../Controllers/Steering/ObstacleAvoidanceController.h"
#include "../../Controllers/Steering/ArriveController.h"
#include "../../Controllers/DecisionMaking/DecisionTreeController.h"
#include "../../Controllers/DecisionMaking/BehaviourTreeController.h"

#include <tge/application.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/graphics/GraphicsEngine.h>
#include <Input/InputHandler.h>
#include <cstdio>
#include <algorithm>
#ifndef _RETAIL
#include <imgui/imgui.h>
#include <tge/graphics/DX11.h>
#endif

GameWorld05_RobotWars::GameWorld05_RobotWars(bool aEnableBehaviourTree)
    : myEnableBehaviourTree(aEnableBehaviourTree)
{}

void GameWorld05_RobotWars::Init(const CommonUtilities::InputHandler& aInput)
{
    // Use the actual window size so the walls, pool, and spawn positions agree.
    myInput = &aInput;
    const Tga::Vector2ui renderSize = Tga::Application::GetInstance()->GetRenderSize();
    const CommonUtilities::Vector2f resolution = {static_cast<float>(renderSize.x), static_cast<float>(renderSize.y)};
    myArenaBounds = std::make_shared<RectTraversalBounds>(resolution * 0.05f, resolution * 0.95f);

    CreateHealthPool();
    if (myEnableBehaviourTree)
        myBehaviourTree = std::make_unique<RobotWarsBehaviourTree>(myBulletSystem, myCombatSettings);
    ConfigureRobotMovement();
    CreateRobots();
    // Create the actors before the combat controllers borrow their enemies.
    myDecisionTreeCombatController.SetEnemy(myActorManager.GetActor(1));
    myStateMachineCombatController.SetEnemy(myActorManager.GetActor(0));
    myBulletSpriteData.texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Sprites/ninjaStar.png");
    myRobotLabels[0].SetText("Decision Tree");
    myRobotLabels[1].SetText("State Machine");
    myRobotLabels[2].SetText("Sir Killalot");

}

void GameWorld05_RobotWars::CreateRobots()
{
    // Give every robot the same movement and health settings. Their AI differs:
    // robot 1 has decisions, robot 2 has states, and U06 adds a behaviour tree.
    const char* spritePaths[3] = {"Sprites/robot1.png", "Sprites/robot2.png", "Sprites/robot1.png"};
    const std::size_t robotCount = myEnableBehaviourTree ? 3u : 2u;
    for (std::size_t robotIndex = 0; robotIndex < robotCount; ++robotIndex)
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
        else if (robotIndex == 1)
        {
            robot.SetController(std::make_unique<StateMachineController>(
                myStateMachineWanderController, myStateMachineHealingController, myStateMachineCombatController));
        }
        else
        {
            robot.SetController(std::make_unique<BehaviourTreeController>(myBehaviourTree->GetTree()));
            robot.SetColor({0.4f, 1.f, 0.6f, 1.f});
        }
        // Wall avoidance adds steering alongside whichever action the AI chose.
        robot.AddController(std::make_unique<ObstacleAvoidanceController>(myWallAvoidanceSettings, myWallObstacles, myArenaBounds));
    }
}

void GameWorld05_RobotWars::Update(float aTimeDelta)
{
    UpdateBattleInput();
    // Damage keys also work while paused; clear a defeated tree right away.
    if (myBehaviourTree && !myActorManager.GetActor(2).IsAlive())
        myBehaviourTree->GetTree().Reset();
#ifndef _RETAIL
    // Pause freezes actors, bullets, healing and respawn timers together.
    if (myPauseBattle && !myStepBattle) return;
    if (myStepBattle)
    {
        aTimeDelta = 1.f / 60.f;
        myStepBattle = false;
    }
#endif
    if (myBehaviourTree)
    {
        // Choose from the same start-of-frame positions for all three robots.
        myDecisionTreeCombatController.SelectNearestEnemy(myActorManager.GetActor(0), myActorManager);
        myStateMachineCombatController.SelectNearestEnemy(myActorManager.GetActor(1), myActorManager);
        myBehaviourTree->GetCombatController().SelectNearestEnemy(myActorManager.GetActor(2), myActorManager);
        myBehaviourTree->GetCombatController().UpdateCooldown(aTimeDelta);
    }
    // Shot timers keep counting down while a robot heals, wanders, or is dead.
    myDecisionTreeCombatController.UpdateCooldown(aTimeDelta);
    myStateMachineCombatController.UpdateCooldown(aTimeDelta);
    myActorManager.Update(aTimeDelta);
    // Resolve bullet damage before healing, so a fatal hit cannot be undone.
    myBulletSystem.Update(aTimeDelta, *myArenaBounds);
    // A fatal shot must also clear execution before this frame's labels are drawn.
    if (myBehaviourTree && !myActorManager.GetActor(2).IsAlive())
        myBehaviourTree->GetTree().Reset();
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
    DrawDebugUI();
}

// Names and HP identify the robots; the inspection windows hold the details.
void GameWorld05_RobotWars::DrawLabels()
{
#ifndef _RETAIL
    if (!myShowRobotLabels) return;
#endif
    for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
    {
        const Actor& robot = myActorManager.GetActor(index);
        Tga::Text& label = myRobotLabels[index];
        char text[80] = {};
        std::snprintf(text, sizeof(text), "%s | HP %.0f/%.0f",
            GetRobotName(index), robot.GetHealth(), robot.GetMaxHealth());
        label.SetText(text);
        label.SetPosition((robot.GetPosition() + CommonUtilities::Vector2f(-label.GetWidth() * 0.5f, 45.f)).ToTga());
        label.Render();
    }
}

const char* GameWorld05_RobotWars::GetRobotName(std::size_t aRobotIndex) const
{
    const char* names[] = {"Decision Tree", "State Machine", "Sir Killalot"};
    return names[aRobotIndex];
}

const char* GameWorld05_RobotWars::GetRobotActionName(std::size_t aRobotIndex) const
{
    const Actor& robot = myActorManager.GetActor(aRobotIndex);
    if (!robot.IsAlive()) return "Defeated";
    if (aRobotIndex == 0)
    {
        const auto& controller = static_cast<const DecisionTreeController&>(*robot.GetController());
        const ActionNode* action = controller.GetSelectedAction();
        if (action == &myDecisionTreeHealingAction) return "Heal";
        if (action == &myDecisionTreeCombatAction) return "Combat";
        if (action == &myDecisionTreeWanderAction) return "Wander";
        return "Idle";
    }
    if (aRobotIndex == 1)
    {
        const auto& controller = static_cast<const StateMachineController&>(*robot.GetController());
        switch (controller.GetState())
        {
        case RobotState::Heal: return "Heal";
        case RobotState::Combat: return "Combat";
        case RobotState::Wander: return "Wander";
        }
        return "Idle";
    }
    return myBehaviourTree->GetActionName();
}

CombatController& GameWorld05_RobotWars::GetRobotCombatController(std::size_t aRobotIndex)
{
    if (aRobotIndex == 0) return myDecisionTreeCombatController;
    if (aRobotIndex == 1) return myStateMachineCombatController;
    return myBehaviourTree->GetCombatController();
}

void GameWorld05_RobotWars::DrawDebugUI()
{
#ifndef _RETAIL
    // Use the existing ImGui style. Each window can be moved, resized or folded.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float width = (std::min)(360.f, viewport->WorkSize.x - 20.f);
    const float height = viewport->WorkSize.y - 60.f;
    const float battleHeight = height * 0.30f;
    const float robotsHeight = height * (myBehaviourTree ? 0.44f : 0.68f);
    const ImVec2 origin = {viewport->WorkPos.x + viewport->WorkSize.x - width - 10.f, viewport->WorkPos.y + 10.f};
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(origin, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({width, battleHeight}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Robot Wars - Battle"))
    {
        ImGui::Text("%.1f FPS | %.2f ms", ImGui::GetIO().Framerate,
            ImGui::GetIO().Framerate > 0.f ? 1000.f / ImGui::GetIO().Framerate : 0.f);
        ImGui::Text("Robots: %zu | Flying bullets: %zu", myActorManager.GetActorCount(), myBulletSystem.GetBullets().size());
        ImGui::Text("Draw calls: %d", Tga::DX11::GetPreviousDrawCallCount());
        ImGui::Checkbox("Pause", &myPauseBattle);
        if (myPauseBattle)
        {
            ImGui::SameLine();
            if (ImGui::Button("Step 1/60 s")) myStepBattle = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset battle")) ResetBattle();
        if (ImGui::CollapsingHeader("Controls and visuals"))
        {
            ImGui::TextWrapped(myBehaviourTree ? "Space / Enter / Backspace: damage robots 1 / 2 / 3. R resets the battle."
                : "Space / Enter: damage robots 1 / 2. R resets the battle.");
            ImGui::Checkbox("Show names and HP", &myShowRobotLabels);
            ImGui::Checkbox("Show arena walls", &myShowWalls);
        }
        if (ImGui::CollapsingHeader("Shared settings"))
        {
            ImGui::Text("Vision %.0f px | Chase gap %.0f px", myCombatSettings.detectionRadius, myCombatSettings.stoppingDistance);
            ImGui::Text("Shot %.0f HP every %.2f s", myCombatSettings.bulletDamage, myCombatSettings.fireInterval);
            ImGui::Text("Bullet %.0f px/s | Lifetime %.1f s", myCombatSettings.bulletSpeed, myCombatSettings.bulletLifetime);
            ImGui::Text("Pool %.0f px | Healing %.0f HP/s", myHealthPool.radius, myHealthPool.healingPerSecond);
        }
    }
    ImGui::End();

    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos({origin.x, origin.y + battleHeight + 10.f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({width, robotsHeight}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Robot Wars - Robots"))
    {
        for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
        {
            Actor& robot = myActorManager.GetActor(index);
            ImGui::PushID(static_cast<int>(index));
            ImGui::Text("%s: %s", GetRobotName(index), GetRobotActionName(index));
            char health[40] = {};
            std::snprintf(health, sizeof(health), "HP %.0f / %.0f", robot.GetHealth(), robot.GetMaxHealth());
            ImGui::ProgressBar(robot.GetHealth() / robot.GetMaxHealth(), {-1.f, 0.f}, health);
            if (ImGui::CollapsingHeader("Inspect"))
            {
                const CombatController& combat = GetRobotCombatController(index);
                ImGui::Text("Wounded: %s (below %.0f HP)", robot.IsWounded() ? "yes" : "no", robot.GetWoundedThreshold());
                if (!robot.IsAlive()) ImGui::Text("Respawn in %.2f s", robot.GetRespawnTimeRemaining());
                const bool inPool = robot.GetPosition().DistanceSqr(myHealthPool.position) <= myHealthPool.radius * myHealthPool.radius;
                ImGui::Text("Inside pool: %s", inPool ? "yes" : "no");
                ImGui::Text("Position: %.1f, %.1f", robot.GetPosition().x, robot.GetPosition().y);
                ImGui::Text("Velocity: %.1f, %.1f", robot.GetVelocity().x, robot.GetVelocity().y);
                ImGui::Text("Speed: %.1f / %.0f px/s", robot.GetVelocity().Length(), robot.GetMaxSpeed());
                ImGui::Text("Next shot in %.2f s", combat.GetTimeUntilNextShot());
                const Actor* enemy = combat.GetEnemy();
                if (enemy)
                {
                    for (std::size_t enemyIndex = 0; enemyIndex < myActorManager.GetActorCount(); ++enemyIndex)
                        if (enemy == &myActorManager.GetActor(enemyIndex)) ImGui::Text("Target: %s", GetRobotName(enemyIndex));
                    ImGui::Text("Distance: %.1f px | Detected: %s", robot.GetPosition().Distance(enemy->GetPosition()),
                        combat.HasDetectedEnemy(robot) ? "yes" : "no");
                }
                else ImGui::TextUnformatted("Target: none");
                if (ImGui::Button("Damage 25 HP")) robot.TakeDamage(25.f);
                ImGui::SameLine();
                if (ImGui::Button("Heal 25 HP")) robot.Heal(25.f);
                // UI damage happens after Update; clear a defeated BT immediately.
                if (index == 2 && !robot.IsAlive()) myBehaviourTree->GetTree().Reset();
            }
            ImGui::PopID();
        }
    }
    ImGui::End();

    if (myBehaviourTree)
    {
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowPos({origin.x, origin.y + battleHeight + robotsHeight + 20.f}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({width, height - battleHeight - robotsHeight}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Robot Wars - Behaviour Tree"))
        {
            const BehaviourTree& tree = myBehaviourTree->GetTree();
            const Actor& robot = myActorManager.GetActor(2);
            ImGui::Text("Action: %s | %s", GetRobotActionName(2), tree.GetRunningAction() ? "Running" : "Idle");
            ImGui::Text("Root traversals: %zu", tree.GetRootTraversalCount());
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("An unchanged counter means the tree is resuming its running action.");
            if (ImGui::CollapsingHeader("Conditions"))
            {
                ImGui::Text("Wounded: %s", robot.IsWounded() ? "true" : "false");
                ImGui::Text("Enemy detected: %s", myBehaviourTree->GetCombatController().HasDetectedEnemy(robot) ? "true" : "false");
            }
            if (ImGui::CollapsingHeader("Branch order"))
            {
                ImGui::TextUnformatted("Selector");
                ImGui::Indent();
                ImGui::TextUnformatted("Sequence: Wounded -> Heal");
                ImGui::TextUnformatted("Sequence: Enemy detected -> Combat");
                ImGui::TextUnformatted("Wander");
                ImGui::Unindent();
            }
        }
        ImGui::End();
    }
#endif
}

void GameWorld05_RobotWars::DrawWallDebug()
{
#ifndef _RETAIL
    if (!myShowWalls) return;
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

    // Healing actions use Arrive with the pool as a fixed target. Wandering
    // chooses new directions, but healing must keep aiming at the same place.
    ArriveControllerData arriveData;
    arriveData.useRandomTargets = false;
    arriveData.slowingRadius = myHealthPool.radius;
    myDecisionTreeHealingController.SetArriveControllerData(arriveData);
    myDecisionTreeHealingController.SetTargetPosition(myHealthPool.position);
    myStateMachineHealingController.SetArriveControllerData(arriveData);
    myStateMachineHealingController.SetTargetPosition(myHealthPool.position);
    if (myBehaviourTree)
        myBehaviourTree->ConfigureMovement(myHealthPool.position, myHealthPool.radius);
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
}

void GameWorld05_RobotWars::UpdateBattleInput()
{
#ifndef _RETAIL
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard) return;
#endif
    // Pressed checks apply damage once per press, rather than every held frame.
    if (myInput->IsKeyPressed(Keys::R))
        ResetBattle();
    if (myInput->IsKeyPressed(Keys::SPACE))
        myActorManager.GetActor(0).TakeDamage(25.f);
    if (myInput->IsKeyPressed(Keys::RETURN))
        myActorManager.GetActor(1).TakeDamage(25.f);
    if (myBehaviourTree && myInput->IsKeyPressed(Keys::BACK))
        myActorManager.GetActor(2).TakeDamage(25.f);
}

CommonUtilities::Vector2f GameWorld05_RobotWars::GetRobotSpawnPosition(std::size_t aRobotIndex) const
{
    // Start within detection range so the battle begins without waiting for wandering.
    const CommonUtilities::Vector2f arenaSize = myArenaBounds->GetMax() - myArenaBounds->GetMin();
    if (myEnableBehaviourTree)
    {
        const CommonUtilities::Vector2f fractions[3] = {{0.35f, 0.42f}, {0.65f, 0.42f}, {0.5f, 0.7f}};
        const CommonUtilities::Vector2f offset = {arenaSize.x * fractions[aRobotIndex].x, arenaSize.y * fractions[aRobotIndex].y};
        return myArenaBounds->GetMin() + offset;
    }
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
    if (myBehaviourTree)
    {
        myBehaviourTree->GetTree().Reset();
        myBehaviourTree->GetCombatController().ResetCooldown();
    }
    static_cast<DecisionTreeController&>(*myActorManager.GetActor(0).GetController()).Reset();
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
