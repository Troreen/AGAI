#pragma once

#include "BT_Node.h"
#include <cstddef>
#include <vector>

// Children are borrowed and visited in the order they are added.
class BT_CompositeNode : public BT_Node
{
public:
    // Build the tree before running it. Each child belongs to only one parent.
    void AddChild(BT_Node& aChild);
    // After a running child was ticked directly, continue with its result.
    // The caller passes the result from the child at myCurrentChildIndex.
    BT_NodeResult Continue(Actor& aActor, float aDeltaTime, BehaviourTree& aTree,
        BT_NodeResult aChildResult);

protected:
    void Init(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) override;
    void OnReset() override;
    virtual BT_NodeResult ContinueFromChild(Actor& aActor, float aDeltaTime,
        BehaviourTree& aTree, BT_NodeResult aChildResult) = 0;

    std::vector<BT_Node*> myChildren;
    std::size_t myCurrentChildIndex = 0;
};
