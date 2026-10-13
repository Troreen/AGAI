#pragma once

#include "../../DecisionMaking/DecisionNode.h"

class CombatController;

// On the tree's healthy branch, visible enemy means Combat; no enemy means Wander.
class EnemyDetectedDecision final : public DecisionNode
{
public:
    EnemyDetectedDecision(const CombatController& aCombatController, DecisionTreeNode& aCombatAction, DecisionTreeNode& aWanderAction);

protected:
    bool Evaluate(const Actor& aActor) const override;

private:
    const CombatController& myCombatController;
};
