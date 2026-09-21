#include "ObstacleAvoidanceController.h"
#include "../Actor.h"

#include <algorithm>
#include <cmath>

ObstacleAvoidanceController::ObstacleAvoidanceController(const ObstacleAvoidanceSettings& aSettings,
    const std::vector<CircleObstacle>& aObstacles, std::shared_ptr<const RectTraversalBounds> aBounds)
    : mySettings(aSettings)
    , myObstacles(aObstacles)
    , myBounds(std::move(aBounds))
{
}

CommonUtilities::Vector2f ObstacleAvoidanceController::GetSteeringForce(const Actor& aActor,
    std::span<const Actor* const>) const
{
    if (mySettings.weight <= 0.f)
        return {};
    const auto avoidance = EvaluateAvoidance(aActor);
    // A clear forward ray must add no force, including no unwanted braking.
    if (!avoidance.forwardBlocked)
        return {};
    // Redirect current motion; avoidance strength controls turning, not a boost to maximum speed.
    // If every ray is blocked, brake instead of blindly accelerating into an obstacle.
    const auto desiredVelocity = avoidance.foundClearDirection
        ? avoidance.direction * aActor.GetVelocity().Length() : CommonUtilities::Vector2f{};
    return (desiredVelocity - aActor.GetVelocity()) * mySettings.weight;
}

ObstacleAvoidanceResult ObstacleAvoidanceController::EvaluateAvoidance(const Actor& aActor) const
{
    auto forward = aActor.GetVelocity();
    if (forward.LengthSqr() <= 0.0001f)
    {
        const float rotation = aActor.GetSpriteInstanceData().rotation;
        forward = { std::cos(rotation), std::sin(rotation) };
    }
    const float length = (std::max)(mySettings.minimumRayLength,
        aActor.GetVelocity().Length() * mySettings.lookAheadSeconds);
    return ObstacleAvoidance::FindDirection(aActor.GetPosition(), forward, length,
        aActor.GetRadius() + mySettings.clearance, myObstacles, myBounds.get());
}
