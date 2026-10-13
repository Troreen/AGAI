#pragma once

#include "BT_Node.h"

// A question only reads actor data. False is Failure; true is Success.
class BT_LeafNodeCondition : public BT_Node
{
public:
    virtual bool Evaluate(const Actor& aActor) const = 0;

protected:
    BT_NodeResult Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) final;
};
