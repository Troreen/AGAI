#pragma once

#include "BT_CompositeNode.h"

// Every child must succeed. Failure or Running stops the sequence immediately.
class BT_Sequence : public BT_CompositeNode
{
protected:
    BT_NodeResult Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) override;
    BT_NodeResult ContinueFromChild(Actor& aActor, float aDeltaTime,
        BehaviourTree& aTree, BT_NodeResult aChildResult) override;
};
