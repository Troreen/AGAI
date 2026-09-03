#include "SeekerController.h"
#include "Actor.h"

SeekerController::SeekerController(const TargetControllerData& aData)
    : TargetController(aData)
{
}

Tga::Vector2f SeekerController::GetDesiredVelocity(const Actor& aActor) const
{
    Tga::Vector2f toTarget = myTargetPosition - aActor.GetPosition();
    const float distanceToTarget = toTarget.Length();
    if (distanceToTarget <= 0.0001f)
        return {};

    return (toTarget / distanceToTarget) * aActor.GetMaxSpeed();
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
