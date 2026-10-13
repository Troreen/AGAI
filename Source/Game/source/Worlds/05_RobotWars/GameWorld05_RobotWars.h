#pragma once

#include "../GameWorld.h"
#include "../../Actors/Actor.h"
#include "../../Actors/ActorManager.h"
#include "../../Interfaces/TraversalBounds.h"
#include "../../Controllers/Steering/ObstacleAvoidance.h"
#include "HealthPool.h"
#include "WoundedDecision.h"
#include "EnemyDetectedDecision.h"
#include "CombatSettings.h"
#include "BulletSystem.h"
#include "../../Controllers/DecisionMaking/CombatController.h"
#include "../../Controllers/DecisionMaking/StateMachineController.h"
#include "../../DecisionMaking/ActionNode.h"
#include "../../DecisionMaking/DecisionTree.h"
#include "../../Controllers/Steering/WanderController.h"
#include "../../Controllers/Steering/ArriveController.h"
#include "../06_BehaviourTree/RobotWarsBehaviourTree.h"
#include <tge/text/text.h>
#include <array>
#include <memory>
#include <vector>

// Own the shared arena and robots. Each AI chooses an action; Actor moves it,
// then this world checks bullet hits and healing for the finished frame.
class GameWorld05_RobotWars : public GameWorld
{
public:
    explicit GameWorld05_RobotWars(bool aEnableBehaviourTree = false);
    void Init(const CommonUtilities::InputHandler& aInput) override;
    void Update(float aTimeDelta) override;
    void Render() override;

private:
    void CreateRobots();
    void CreateHealthPool();
    void ResetBattle();
    CommonUtilities::Vector2f GetRobotSpawnPosition(std::size_t aRobotIndex) const;
    void DrawBullets();
    void ConfigureRobotMovement();
    void UpdateBattleInput();
    void UpdateHealthPool(float aDeltaTime);
    void DrawHealthPool();
    void DrawLabels();
    void DrawWallDebug();
    void DrawDebugUI();
    const char* GetRobotName(std::size_t aRobotIndex) const;
    const char* GetRobotActionName(std::size_t aRobotIndex) const;
    CombatController& GetRobotCombatController(std::size_t aRobotIndex);

    ObstacleAvoidanceSettings myWallAvoidanceSettings;
    std::vector<CircleObstacle> myWallObstacles;
    HealthPool myHealthPool;
    CombatSettings myCombatSettings;
    BulletSystem myBulletSystem;
    // Robot 1's tree: wounded -> Heal, otherwise enemy detected -> Combat or Wander.
    // These nodes borrow their children and controllers rather than owning them.
    WanderController myDecisionTreeWanderController;
    ArriveController myDecisionTreeHealingController;
    CombatController myDecisionTreeCombatController{myBulletSystem, myCombatSettings};
    ActionNode myDecisionTreeCombatAction{myDecisionTreeCombatController};
    ActionNode myDecisionTreeWanderAction{myDecisionTreeWanderController};
    ActionNode myDecisionTreeHealingAction{myDecisionTreeHealingController};
    EnemyDetectedDecision myDecisionTreeEnemyDetectedDecision{myDecisionTreeCombatController, myDecisionTreeCombatAction, myDecisionTreeWanderAction};
    WoundedDecision myDecisionTreeWoundedDecision{myDecisionTreeHealingAction, myDecisionTreeEnemyDetectedDecision};
    DecisionTree myDecisionTree{myDecisionTreeWoundedDecision};
    // Robot 2 uses the same actions, but needs its own wander state and shot timer.
    WanderController myStateMachineWanderController;
    ArriveController myStateMachineHealingController;
    CombatController myStateMachineCombatController{myBulletSystem, myCombatSettings};
    std::unique_ptr<RobotWarsBehaviourTree> myBehaviourTree;
    // Members are destroyed from bottom to top. Destroy actors before the
    // controllers and tree nodes they borrow.
    ActorManager myActorManager;
    std::shared_ptr<RectTraversalBounds> myArenaBounds;
    std::array<Tga::Text, 3> myRobotLabels;
    Tga::SpriteSharedData myHealthPoolSpriteData = {};
    Tga::Sprite2DInstanceData myHealthPoolSprite = {};
    Tga::SpriteSharedData myBulletSpriteData = {};
    Tga::Text myHealthPoolLabel;
#ifndef _RETAIL
    bool myPauseBattle = false;
    bool myStepBattle = false;
    bool myShowWalls = false;
    bool myShowRobotLabels = true;
#endif
    const bool myEnableBehaviourTree;
    const CommonUtilities::InputHandler* myInput = nullptr;
};
