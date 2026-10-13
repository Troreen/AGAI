#include "BT_Sequence.h"

BT_NodeResult BT_Sequence::Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree)
{
    while (myCurrentChildIndex < myChildren.size())
    {
        const BT_NodeResult result = myChildren[myCurrentChildIndex]->Tick(aActor, aDeltaTime, aTree);
        if (result != BT_NodeResult::Success)
        {
            return result;
        }
        ++myCurrentChildIndex;
    }
    return BT_NodeResult::Success;
}

BT_NodeResult BT_Sequence::ContinueFromChild(Actor& aActor, float aDeltaTime,
    BehaviourTree& aTree, BT_NodeResult aChildResult)
{
    if (aChildResult != BT_NodeResult::Success)
    {
        return aChildResult;
    }
    // This child already finished. Visit the next one without replaying it.
    ++myCurrentChildIndex;
    return Update(aActor, aDeltaTime, aTree);
}
