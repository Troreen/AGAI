#pragma once

#include "BT_WoundedCondition.h"
#include "BT_EnemyDetectedCondition.h"
#include "../../DecisionMaking/BehaviourTree/BT_Selector.h"
#include "../../DecisionMaking/BehaviourTree/BT_Sequence.h"
#include "../../DecisionMaking/BehaviourTree/BT_ControllerAction.h"
#include "../../DecisionMaking/BehaviourTree/BehaviourTree.h"
#include "../../Controllers/Steering/WanderController.h"
#include "../../Controllers/Steering/ArriveController.h"
#include "../../Controllers/DecisionMaking/CombatController.h"

// One robot's entire tree and actions. The world owns this bundle before actors,
// so all the borrowed nodes and controllers stay alive until the robot is gone.
class RobotWarsBehaviourTree
{
public:
    RobotWarsBehaviourTree(BulletSystem& aBullets, const CombatSettings& aSettings);
    void ConfigureMovement(const CommonUtilities::Vector2f& aPoolPosition, float aPoolRadius);
    BehaviourTree& GetTree();
    const BehaviourTree& GetTree() const;
    CombatController& GetCombatController();
    const char* GetActionName() const;

private:
    WanderController myWanderController;
    ArriveController myHealingController;
    CombatController myCombatController;
    BT_WoundedCondition myWoundedCondition;
    BT_EnemyDetectedCondition myEnemyDetectedCondition;
    BT_ControllerAction myHealingAction;
    BT_ControllerAction myCombatAction;
    BT_ControllerAction myWanderAction;
    BT_Sequence myHealingSequence;
    BT_Sequence myCombatSequence;
    BT_Selector myRoot;
    BehaviourTree myTree;
};
