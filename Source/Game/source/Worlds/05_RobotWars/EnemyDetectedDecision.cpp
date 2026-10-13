#include "EnemyDetectedDecision.h"
#include "../../Controllers/DecisionMaking/CombatController.h"

EnemyDetectedDecision::EnemyDetectedDecision(const CombatController& aCombatController, DecisionTreeNode& aCombatAction, DecisionTreeNode& aWanderAction)
    : DecisionNode(aCombatAction, aWanderAction), myCombatController(aCombatController)
{}

bool EnemyDetectedDecision::Evaluate(const Actor& aActor) const
{
    // Use the same vision check as combat so choosing and executing agree.
    return myCombatController.HasDetectedEnemy(aActor);
}
