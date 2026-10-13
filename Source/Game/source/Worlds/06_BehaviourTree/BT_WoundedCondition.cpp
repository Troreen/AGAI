#include "BT_WoundedCondition.h"
#include "../../Actors/Actor.h"

bool BT_WoundedCondition::Evaluate(const Actor& aActor) const
{
    return aActor.IsWounded();
}
