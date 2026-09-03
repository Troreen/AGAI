#include "WanderController.h"
#include "Actor.h"

#include <algorithm>
#include <cmath>

WanderController::WanderController(const WanderControllerData& aData)
    : Controller(aData)
    , myWanderData(aData)
    , myRandomGenerator(aData.randomSeed)
{
    SetWanderControllerData(aData);
}

Tga::Vector2f WanderController::GetDesiredVelocity(const Actor& aActor) const
{
    const Tga::Vector2f toWanderTarget = GetWanderTarget() - aActor.GetPosition();
    const float distance = toWanderTarget.Length();
    if (distance <= 0.0001f)
        return {};

    return (toWanderTarget / distance) * aActor.GetMaxSpeed();
}

void WanderController::Update(Actor& aActor, float aDeltaTime)
{
    Tga::Vector2f forward = aActor.GetVelocity();
    if (forward.LengthSqr() <= 0.0001f)
        forward = { 1.f, 0.f };
    else
        forward = forward.GetNormalized();

    myJitterTimer += aDeltaTime;
    if (myJitterTimer >= myWanderData.directionChangeInterval)
    {
        myCurrentJitter = GenerateTurnRate();
        myJitterTimer = std::fmod(myJitterTimer, myWanderData.directionChangeInterval);
    }

    myWanderTargetAngle += myCurrentJitter * aDeltaTime;
    myWanderCircleCenter = aActor.GetPosition() + forward * myWanderData.circleDistance;
}

Tga::Vector2f WanderController::GetWanderTarget() const
{
    return myWanderCircleCenter + Tga::Vector2f(
        cosf(myWanderTargetAngle), sinf(myWanderTargetAngle)) * myWanderData.circleRadius;
}

float WanderController::GenerateTurnRate()
{
    std::uniform_real_distribution<float> distribution(-myWanderData.maxTurnRate, myWanderData.maxTurnRate);
    return distribution(myRandomGenerator);
}

ControllerDebugInfo WanderController::GetDebugInfo() const
{
    ControllerDebugInfo info;
    info.wanderCircleRadius = myWanderData.circleRadius;
    info.wanderCircleDistance = myWanderData.circleDistance;
    info.wanderCircleCenter = myWanderCircleCenter;
    info.wanderTargetAngle = myWanderTargetAngle;
    return info;
}

const WanderControllerData& WanderController::GetWanderControllerData() const { return myWanderData; }

void WanderController::SetWanderControllerData(const WanderControllerData& aData)
{
    myWanderData = aData;
    myWanderData.circleRadius = (std::max)(0.f, myWanderData.circleRadius);
    myWanderData.circleDistance = (std::max)(0.f, myWanderData.circleDistance);
    myWanderData.maxTurnRate = (std::max)(0.f, myWanderData.maxTurnRate);
    myWanderData.directionChangeInterval = (std::max)(0.01f, myWanderData.directionChangeInterval);
    SetControllerData(myWanderData);
}
