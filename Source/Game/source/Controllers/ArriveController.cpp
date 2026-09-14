#include "ArriveController.h"
#include "../Actor.h"
#include "ControllerUtils.h"

#include <algorithm>
#include <cmath>

ArriveController::ArriveController(const ArriveControllerData& aData)
    : TargetController(aData)
{
    SetArriveControllerData(aData);
}

CommonUtilities::Vector2f ArriveController::GetDesiredVelocity(const Actor& aActor) const
{
    return ControllerUtils::ArriveDesiredVelocity(aActor, myTargetPosition, myArriveData.slowingRadius);
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
