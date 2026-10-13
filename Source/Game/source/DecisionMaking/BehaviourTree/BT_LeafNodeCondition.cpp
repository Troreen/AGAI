#include "BT_LeafNodeCondition.h"

BT_NodeResult BT_LeafNodeCondition::Update(Actor& aActor, float, BehaviourTree&)
{
    return Evaluate(aActor) ? BT_NodeResult::Success : BT_NodeResult::Failure;
}
