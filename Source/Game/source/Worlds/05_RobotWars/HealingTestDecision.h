#pragma once

#include "../../DecisionMaking/DecisionNode.h"

// H supplies a manual condition while the tree framework is being tested.
class HealingTestDecision final : public DecisionNode
{
public:
    HealingTestDecision(const bool& aSeekHealthPool, DecisionTreeNode& aHealingAction, DecisionTreeNode& aWanderAction);

protected:
    bool Evaluate(const Actor& aActor) const override;

private:
    const bool& mySeekHealthPool;
};
