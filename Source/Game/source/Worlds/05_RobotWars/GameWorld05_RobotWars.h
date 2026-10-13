#pragma once

#include "../GameWorld.h"
#include "../../Actors/Actor.h"
#include "../../Actors/ActorManager.h"
#include "../../Interfaces/TraversalBounds.h"
#include "../../Controllers/Steering/ObstacleAvoidance.h"
#include "HealthPool.h"
#include "WoundedDecision.h"
#include "../../DecisionMaking/ActionNode.h"
#include "../../DecisionMaking/DecisionTree.h"
#include "../../Controllers/Steering/WanderController.h"
#include "../../Controllers/Steering/ArriveController.h"
#include <tge/text/text.h>
#include <array>
#include <memory>
#include <vector>

class GameWorld05_RobotWars final : public GameWorld
{
public:
    void Init(const CommonUtilities::InputHandler& aInput) override;
    void Update(float aTimeDelta) override;
    void Render() override;

private:
    void CreateRobots();
    void CreateHealthPool();
    void ConfigureDecisionTree();
    void UpdateTestInput();
    void UpdateHealthPool(float aDeltaTime);
    void DrawHealthPool();
    void DrawLabels();
    void DrawWallDebug();

    ObstacleAvoidanceSettings myWallAvoidanceSettings;
    std::vector<CircleObstacle> myWallObstacles;
    HealthPool myHealthPool;
    // Keep controllers and nodes alive for as long as the actor borrows the tree.
    WanderController myDecisionTreeWanderController;
    ArriveController myDecisionTreeArriveController;
    ActionNode myWanderAction{myDecisionTreeWanderController};
    ActionNode myHealingAction{myDecisionTreeArriveController};
    WoundedDecision myWoundedDecision{myHealingAction, myWanderAction};
    DecisionTree myRobotDecisionTree{myWoundedDecision};
    ActorManager myActorManager;
    std::shared_ptr<RectTraversalBounds> myArenaBounds;
    std::array<Tga::Text, 2> myRobotLabels;
    Tga::SpriteSharedData myHealthPoolSpriteData = {};
    Tga::Sprite2DInstanceData myHealthPoolSprite = {};
    Tga::Text myHealthPoolLabel;
    Tga::Text myControlsLabel;
    const CommonUtilities::InputHandler* myInput = nullptr;
};
