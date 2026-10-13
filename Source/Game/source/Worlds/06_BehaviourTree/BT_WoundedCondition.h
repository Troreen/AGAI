#pragma once
#include "../../DecisionMaking/BehaviourTree/BT_LeafNodeCondition.h"

class BT_WoundedCondition final : public BT_LeafNodeCondition
{
public:
    bool Evaluate(const Actor& aActor) const override;
};
