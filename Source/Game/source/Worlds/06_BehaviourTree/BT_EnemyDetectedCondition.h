#pragma once
#include "../../DecisionMaking/BehaviourTree/BT_LeafNodeCondition.h"

class CombatController;

class BT_EnemyDetectedCondition final : public BT_LeafNodeCondition
{
public:
    explicit BT_EnemyDetectedCondition(const CombatController& aCombatController);
    bool Evaluate(const Actor& aActor) const override;

private:
    const CombatController& myCombatController;
};
