#include "BT_EnemyDetectedCondition.h"
#include "../../Controllers/DecisionMaking/CombatController.h"

BT_EnemyDetectedCondition::BT_EnemyDetectedCondition(const CombatController& aCombatController)
    : myCombatController(aCombatController)
{}

bool BT_EnemyDetectedCondition::Evaluate(const Actor& aActor) const
{
    return myCombatController.HasDetectedEnemy(aActor);
}
