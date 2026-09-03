#include "ArriveController.h"
#include "Actor.h"

#include <algorithm>
#include <cmath>

ArriveController::ArriveController(const ArriveControllerData& aData)
    : TargetController(aData)
{
    SetArriveControllerData(aData);
}

Tga::Vector2f ArriveController::GetDesiredVelocity(const Actor& aActor) const
{
    Tga::Vector2f toTarget = myTargetPosition - aActor.GetPosition();
    const float distanceToTarget = toTarget.Length();
    if (distanceToTarget <= 0.0001f)
    {
        return {};
    }

    // remap the desired speed based on the distance to the target and the slowing radius
    // remap takes a value from one numerical range and converts it proportionally into another numerical range.
    float desiredSpeed = aActor.GetMaxSpeed();
    if (distanceToTarget < myArriveData.slowingRadius)
    {
        const float slowDownFactor = distanceToTarget / myArriveData.slowingRadius;
        desiredSpeed *= slowDownFactor;
    }
    return (toTarget / distanceToTarget) * desiredSpeed;
}

void ArriveController::Update(Actor& aActor, float aDeltaTime)
{
    KeepTargetReachable(aActor);
    myTargetTimer += aDeltaTime;
    if (myTargetTimer >= myArriveData.targetChangeInterval)
    {
        SetRandomTarget(aActor);
        myTargetTimer = std::fmod(myTargetTimer, myArriveData.targetChangeInterval);
    }
}

ControllerDebugInfo ArriveController::GetDebugInfo() const
{
    ControllerDebugInfo info;
    info.hasTarget = true;
    info.targetPosition = myTargetPosition;
    info.hasSlowingRadius = true;
    info.slowingRadius = myArriveData.slowingRadius;
    return info;
}

const ArriveControllerData& ArriveController::GetArriveControllerData() const { return myArriveData; }

void ArriveController::SetArriveControllerData(const ArriveControllerData& aData)
{
    myArriveData = aData;
    myArriveData.slowingRadius = (std::max)(0.01f, myArriveData.slowingRadius);
    myArriveData.targetChangeInterval = (std::max)(0.01f, myArriveData.targetChangeInterval);
    SetControllerData(myArriveData);
}
