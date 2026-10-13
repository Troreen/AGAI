#include "BehaviourTreeController.h"
#include "../../DecisionMaking/BehaviourTree/BehaviourTree.h"
#include "../../DecisionMaking/BehaviourTree/BT_LeafNodeAction.h"

BehaviourTreeController::BehaviourTreeController(BehaviourTree& aTree) : myTree(aTree)
{}

void BehaviourTreeController::Update(Actor& aActor, float aDeltaTime)
{
    myTree.Run(aActor, aDeltaTime);
}

CommonUtilities::Vector2f BehaviourTreeController::GetDesiredVelocity(const Actor& aActor) const
{
    const BT_LeafNodeAction* action = myTree.GetRunningAction();
    return action ? action->GetDesiredVelocity(aActor) : CommonUtilities::Vector2f{};
}
