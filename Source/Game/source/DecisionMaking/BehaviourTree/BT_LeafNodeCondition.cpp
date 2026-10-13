#include "BT_LeafNodeCondition.h"
#include "BehaviourTree.h"

BT_NodeResult BT_LeafNodeCondition::Update(Actor& aActor, float, BehaviourTree& aTree)
{
    const bool result = Evaluate(aActor);
    aTree.RememberCondition(*this, result);
    return result ? BT_NodeResult::Success : BT_NodeResult::Failure;
}
