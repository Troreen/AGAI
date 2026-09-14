#include "SeekerController.h"
#include "../Actor.h"
#include "ControllerUtils.h"

SeekerController::SeekerController(const TargetControllerData& aData)
    : TargetController(aData)
{
}

CommonUtilities::Vector2f SeekerController::GetDesiredVelocity(const Actor& aActor) const
{
    return ControllerUtils::SeekDesiredVelocity(aActor, myTargetPosition);
}

void SeekerController::Update(Actor& aActor, float)
{
    KeepTargetReachable(aActor);
    const float targetReachedRadius = GetTargetControllerData().targetReachedRadius;
    if ((myTargetPosition - aActor.GetPosition()).LengthSqr() < targetReachedRadius * targetReachedRadius)
        SetRandomTarget(aActor);
}

ControllerDebugInfo SeekerController::GetDebugInfo() const
{
    ControllerDebugInfo info;
    info.hasTarget = true;
    info.targetPosition = myTargetPosition;
    return info;
}
