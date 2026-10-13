#include "BT_Selector.h"

BT_NodeResult BT_Selector::Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree)
{
    while (myCurrentChildIndex < myChildren.size())
    {
        const BT_NodeResult result = myChildren[myCurrentChildIndex]->Tick(aActor, aDeltaTime, aTree);
        if (result != BT_NodeResult::Failure)
        {
            return result;
        }
        ++myCurrentChildIndex;
    }
    return BT_NodeResult::Failure;
}

BT_NodeResult BT_Selector::ContinueFromChild(Actor& aActor, float aDeltaTime,
    BehaviourTree& aTree, BT_NodeResult aChildResult)
{
    if (aChildResult != BT_NodeResult::Failure)
    {
        return aChildResult;
    }
    // This branch already failed. Try the next one without replaying it.
    ++myCurrentChildIndex;
    return Update(aActor, aDeltaTime, aTree);
}
