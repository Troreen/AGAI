#include "BT_LeafNodeAction.h"
#include "BehaviourTree.h"

BT_NodeResult BT_LeafNodeAction::Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree)
{
    aTree.RememberAction(*this);
    return Execute(aActor, aDeltaTime, aTree);
}

CommonUtilities::Vector2f BT_LeafNodeAction::GetDesiredVelocity(const Actor&) const
{
    return {};
}
