#pragma once

#include "../../DecisionMaking/DecisionNode.h"

// Healing is checked before the robot considers its normal behaviour.
class WoundedDecision final : public DecisionNode
{
public:
    WoundedDecision(DecisionTreeNode& aHealingAction, DecisionTreeNode& aNormalBehaviour);

protected:
    bool Evaluate(const Actor& aActor) const override;
};
