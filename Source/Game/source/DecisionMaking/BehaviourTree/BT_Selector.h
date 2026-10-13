#pragma once

#include "BT_CompositeNode.h"

// The first Success or Running child is selected. Failure tries the next child.
class BT_Selector : public BT_CompositeNode
{
protected:
    BT_NodeResult Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) override;
    BT_NodeResult ContinueFromChild(Actor& aActor, float aDeltaTime,
        BehaviourTree& aTree, BT_NodeResult aChildResult) override;
};
