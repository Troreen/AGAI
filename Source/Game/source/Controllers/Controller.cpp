#include "Controller.h"
#include "Actor.h"
#include "TraversalBounds.h"

#include <tge/application.h>

#include <algorithm>
#include <random>

Controller::Controller(const ControllerData& aData)
    : myData(aData)
{
}

Tga::Vector2f Controller::GetSteeringForce(const Actor& aActor) const
{
    const Tga::Vector2f primaryForce =
        (GetDesiredVelocity(aActor) - aActor.GetVelocity()) * myData.behaviorWeight;

    bool isOutsideBounds = false;
    const Tga::Vector2f containmentForce = GetContainmentForce(aActor, isOutsideBounds);
    if (isOutsideBounds)
        return containmentForce;

    return primaryForce + containmentForce * myData.containmentWeight;
}

const ControllerData& Controller::GetControllerData() const
{
    return myData;
}

void Controller::SetControllerData(const ControllerData& aData)
{
    myData = aData;
    myData.behaviorWeight = (std::max)(0.f, myData.behaviorWeight);
    myData.containmentWeight = (std::max)(0.f, myData.containmentWeight);
    myData.boundaryLookAhead = (std::max)(0.f, myData.boundaryLookAhead);
    myData.boundaryClearance = (std::max)(0.f, myData.boundaryClearance);
}

void Controller::SetTraversalBounds(std::shared_ptr<const ITraversalBounds> aBounds)
{
    myTraversalBounds = std::move(aBounds);
}

Tga::Vector2f Controller::GetContainmentForce(const Actor& aActor, bool& aIsOutsideBounds) const
{
    aIsOutsideBounds = false;
    if (!myData.useContainment || !myTraversalBounds)
        return {};

    const Tga::Vector2f position = aActor.GetPosition();
    const Tga::Vector2f predictedPosition = position + aActor.GetVelocity() * myData.boundaryLookAhead;
    const float inset = aActor.GetRadius() + myData.boundaryClearance;
    const Tga::Vector2f recovery = myTraversalBounds->GetRecoveryDirection(position, predictedPosition, inset);
    if (recovery.LengthSqr() <= 0.0001f)
        return {};

    // A current-position query distinguishes a gentle predictive correction
    // from an emergency recovery when the actor has already escaped.
    aIsOutsideBounds = myTraversalBounds->GetRecoveryDirection(position, position, inset).LengthSqr() > 0.0001f;
    return recovery.GetNormalized() * aActor.GetMaxSpeed() - aActor.GetVelocity();
}

Tga::Vector2f Controller::GetNearestValidPosition(const Actor& aActor, const Tga::Vector2f& aPosition) const
{
    if (!myTraversalBounds)
        return aPosition;

    return myTraversalBounds->GetNearestValidPoint(
        aPosition, aActor.GetRadius() + myData.boundaryClearance);
}

ContainmentDebugInfo Controller::GetContainmentDebugInfo(const Actor& aActor) const
{
    ContainmentDebugInfo info;
    info.actorRadius = aActor.GetRadius();
    info.boundaryClearance = myData.boundaryClearance;
    if (!myData.useContainment || !myTraversalBounds)
        return info;

    info.isEnabled = true;
    info.predictedPosition = aActor.GetPosition() + aActor.GetVelocity() * myData.boundaryLookAhead;
    info.nearestValidPoint = myTraversalBounds->GetNearestValidPoint(
        info.predictedPosition, info.actorRadius + info.boundaryClearance);
    info.needsCorrection = (info.nearestValidPoint - info.predictedPosition).LengthSqr() > 0.0001f;
    return info;
}

TargetController::TargetController(const TargetControllerData& aData)
    : Controller(aData)
    , myTargetData(aData)
    , myRandomGenerator(aData.randomSeed)
    , myTargetPosition(GenerateRandomTarget())
{
}

void TargetController::SetTargetPosition(const Tga::Vector2f& aPosition)
{
    myTargetPosition = aPosition;
}

void TargetController::SetRandomTarget()
{
    myTargetPosition = GenerateRandomTarget();
}

void TargetController::SetRandomTarget(const Actor& aActor)
{
    myTargetPosition = GetNearestValidPosition(aActor, GenerateRandomTarget());
}

void TargetController::KeepTargetReachable(const Actor& aActor)
{
    myTargetPosition = GetNearestValidPosition(aActor, myTargetPosition);
}

const Tga::Vector2f& TargetController::GetTargetPosition() const
{
    return myTargetPosition;
}

const TargetControllerData& TargetController::GetTargetControllerData() const
{
    return myTargetData;
}

void TargetController::SetTargetControllerData(const TargetControllerData& aData)
{
    myTargetData = aData;
    myTargetData.targetReachedRadius = (std::max)(0.f, myTargetData.targetReachedRadius);
    SetControllerData(myTargetData);
}

Tga::Vector2f TargetController::GenerateRandomTarget()
{
    const Tga::Vector2ui resolution = Tga::Application::GetInstance()->GetRenderSize();
    std::uniform_real_distribution<float> xDistribution(0.f, static_cast<float>(resolution.x));
    std::uniform_real_distribution<float> yDistribution(0.f, static_cast<float>(resolution.y));
    return { xDistribution(myRandomGenerator), yDistribution(myRandomGenerator) };
}



