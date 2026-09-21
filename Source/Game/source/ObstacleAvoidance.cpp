#include "ObstacleAvoidance.h"
#include "Interfaces/TraversalBounds.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
    const std::array<CommonUtilities::Vector2f, ObstacleAvoidanceResult::rayCount>& GetDirections()
    {
        // Inspired by SebLague/Boids' golden-ratio sampling, adapted from a sphere to a circle.
        // https://github.com/SebLague/Boids/blob/master/Assets/Scripts/BoidHelper.cs
        // The irrational turn spreads samples around the circle without repeating directions.
        static const auto directions = []
        {
            std::array<CommonUtilities::Vector2f, ObstacleAvoidanceResult::rayCount> result;
            const float goldenAngle = std::numbers::pi_v<float> * (3.f - std::sqrt(5.f));
            result[0] = { 1.f, 0.f };
            for (std::size_t index = 1; index < result.size(); ++index)
            {
                const float angle = goldenAngle * static_cast<float>(index);
                result[index] = { std::cos(angle), std::sin(angle) };
            }
            // Prefer a small turn before considering directions behind the fish.
            std::sort(result.begin() + 1, result.end(), [](const auto& aLeft, const auto& aRight)
                { return aLeft.x > aRight.x; });
            return result;
        }();
        return directions;
    }
}

AvoidanceRay ObstacleAvoidance::CastRay(const CommonUtilities::Vector2f& aPosition,
    const CommonUtilities::Vector2f& aDirection, float aLength, float aRadius,
    std::span<const CircleObstacle> aObstacles, const RectTraversalBounds* aBounds)
{
    AvoidanceRay ray;
    ray.direction = aDirection;
    ray.distance = (std::max)(0.f, aLength);
    if (aBounds)
    {
        // Shrink the room by the fish's radius plus clearance, so its body avoids the walls.
        const float radius = (std::max)(0.f, aRadius);
        const auto min = aBounds->GetMin() + CommonUtilities::Vector2f{ radius, radius };
        const auto max = aBounds->GetMax() - CommonUtilities::Vector2f{ radius, radius };
        if (min.x >= max.x || min.y >= max.y)
        {
            ray.blocked = true;
            ray.distance = 0.f;
            return ray;
        }
        const auto checkWall = [&](float aInsideDistance, float aInwardDirection)
        {
            // A fish already beyond an edge may steer back in, but never further out.
            if (aInsideDistance < 0.f && aInwardDirection <= 0.f)
            {
                ray.blocked = true;
                ray.distance = 0.f;
            }
            else if (aInsideDistance >= 0.f && aInwardDirection < 0.f)
            {
                const float distance = aInsideDistance / -aInwardDirection;
                if (distance <= ray.distance)
                {
                    ray.blocked = true;
                    ray.distance = distance;
                }
            }
        };
        checkWall(aPosition.x - min.x, aDirection.x);
        checkWall(max.x - aPosition.x, -aDirection.x);
        checkWall(aPosition.y - min.y, aDirection.y);
        checkWall(max.y - aPosition.y, -aDirection.y);
    }
    for (const CircleObstacle& obstacle : aObstacles)
    {
        const float radius = (std::max)(0.f, obstacle.radius) + (std::max)(0.f, aRadius);
        const auto offset = aPosition - obstacle.position;
        const float projection = offset.Dot(aDirection);
        const float outsideDistanceSqr = offset.LengthSqr() - radius * radius;

        if (outsideDistanceSqr <= 0.f)
        {
            // If already overlapping, allow movement outwards instead of trapping the fish.
            if (projection > 0.f || offset.LengthSqr() <= 0.0001f)
                continue;
            ray.distance = 0.f;
            ray.blocked = true;
            continue;
        }

        const float discriminant = projection * projection - outsideDistanceSqr;
        if (discriminant < 0.f)
            continue;
        const float hitDistance = -projection - std::sqrt(discriminant);
        if (hitDistance >= 0.f && hitDistance <= ray.distance)
        {
            ray.distance = hitDistance;
            ray.blocked = true;
        }
    }
    return ray;
}

ObstacleAvoidanceResult ObstacleAvoidance::FindDirection(const CommonUtilities::Vector2f& aPosition,
    const CommonUtilities::Vector2f& aForward, float aLength, float aRadius,
    std::span<const CircleObstacle> aObstacles, const RectTraversalBounds* aBounds)
{
    ObstacleAvoidanceResult result;
    const auto forward = aForward.LengthSqr() > 0.0001f
        ? aForward.GetNormalized() : CommonUtilities::Vector2f{ 1.f, 0.f };
    for (const auto& localDirection : GetDirections())
    {
        const CommonUtilities::Vector2f direction = {
            forward.x * localDirection.x - forward.y * localDirection.y,
            forward.y * localDirection.x + forward.x * localDirection.y };
        const auto ray = CastRay(aPosition, direction, aLength, aRadius, aObstacles, aBounds);
        result.rays[result.testedRayCount++] = ray;
        if (result.testedRayCount == 1)
            result.forwardBlocked = ray.blocked;
        if (!ray.blocked)
        {
            result.foundClearDirection = true;
            result.direction = direction;
            break;
        }
    }
    return result;
}
