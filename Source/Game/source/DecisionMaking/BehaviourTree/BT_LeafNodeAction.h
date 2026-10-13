#pragma once

#include "BT_Node.h"
#include <Math/Vector2.hpp>

// Update performs the action and reports whether it is still Running.
// Actor asks for movement after the selected action has been updated.
class BT_LeafNodeAction : public BT_Node
{
public:
    // Actions without movement can keep this default zero velocity.
    virtual CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const;

protected:
    BT_NodeResult Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) final;
    virtual BT_NodeResult Execute(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) = 0;
};
