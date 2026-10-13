#include "RobotWarsBehaviourTree.h"

RobotWarsBehaviourTree::RobotWarsBehaviourTree(BulletSystem& aBullets, const CombatSettings& aSettings)
    : myCombatController(aBullets, aSettings), myEnemyDetectedCondition(myCombatController),
      myHealingAction(myHealingController), myCombatAction(myCombatController),
      myWanderAction(myWanderController), myTree(myRoot)
{
    // The Selector checks priorities in this order: Heal, Combat, Wander.
    myHealingSequence.AddChild(myWoundedCondition);
    myHealingSequence.AddChild(myHealingAction);
    myCombatSequence.AddChild(myEnemyDetectedCondition);
    myCombatSequence.AddChild(myCombatAction);
    myRoot.AddChild(myHealingSequence);
    myRoot.AddChild(myCombatSequence);
    myRoot.AddChild(myWanderAction);
}

void RobotWarsBehaviourTree::ConfigureMovement(const CommonUtilities::Vector2f& aPoolPosition, float aPoolRadius)
{
    WanderControllerData wanderData;
    wanderData.randomSeed = 3u;
    myWanderController.SetWanderControllerData(wanderData);
    ArriveControllerData arriveData;
    arriveData.useRandomTargets = false;
    arriveData.slowingRadius = aPoolRadius;
    myHealingController.SetArriveControllerData(arriveData);
    myHealingController.SetTargetPosition(aPoolPosition);
}

BehaviourTree& RobotWarsBehaviourTree::GetTree() { return myTree; }
const BehaviourTree& RobotWarsBehaviourTree::GetTree() const { return myTree; }
CombatController& RobotWarsBehaviourTree::GetCombatController() { return myCombatController; }

const char* RobotWarsBehaviourTree::GetActionName() const
{
    const BT_LeafNodeAction* action = myTree.GetRunningAction();
    if (action == &myHealingAction) return "Heal";
    if (action == &myCombatAction) return "Combat";
    if (action == &myWanderAction) return "Wander";
    return "Idle";
}
