#include "HealingTestDecision.h"

HealingTestDecision::HealingTestDecision(const bool& aSeekHealthPool, DecisionTreeNode& aHealingAction, DecisionTreeNode& aWanderAction)
    : DecisionNode(aHealingAction, aWanderAction), mySeekHealthPool(aSeekHealthPool)
{}

bool HealingTestDecision::Evaluate(const Actor& /*aActor*/) const
{
    return mySeekHealthPool;
}
