#pragma once

#include <Vector2.hpp>
#include <array>
#include <span>

class RectTraversalBounds;

struct CircleObstacle
{
    CommonUtilities::Vector2f position = {};
    // Radius of the solid obstacle in world units.
    float radius = 40.f;
};

struct ObstacleAvoidanceSettings
{
    // Minimum distance checked ahead, even when the fish is moving slowly.
    float minimumRayLength = 100.f;
    // Seconds of travel checked ahead. Fast fish cast longer rays to turn earlier.
    float lookAheadSeconds = 0.75f;
    // Extra empty space kept between the fish's collision radius and an obstacle.
    float clearance = 10.f;
    // Strength of the avoidance steering compared with flocking; zero disables avoidance.
    float weight = 10.f;
};

struct AvoidanceRay
{
    CommonUtilities::Vector2f direction = {};
    float distance = 0.f;
    bool blocked = false;
};

struct ObstacleAvoidanceResult
{
    // Maximum number of directions to try. More directions can find narrower escape routes.
    static constexpr std::size_t rayCount = 64;
    std::array<AvoidanceRay, rayCount> rays = {};
    std::size_t testedRayCount = 0;
    bool forwardBlocked = false;
    bool foundClearDirection = false;
    CommonUtilities::Vector2f direction = {};
};

namespace ObstacleAvoidance
{
    // Cast against circles enlarged by the fish's radius: the 2D equivalent of a sphere cast.
    // Directions must be normalized. Hit distance is measured from the ray's origin.
    // When bounds are supplied, their four edges are walls enclosing the navigable area.
    AvoidanceRay CastRay(const CommonUtilities::Vector2f& aPosition,
        const CommonUtilities::Vector2f& aDirection, float aLength, float aRadius,
        std::span<const CircleObstacle> aObstacles, const RectTraversalBounds* aBounds = nullptr);

    // Forward first, then golden-angle samples ordered from small turns to large turns.
    ObstacleAvoidanceResult FindDirection(const CommonUtilities::Vector2f& aPosition,
        const CommonUtilities::Vector2f& aForward, float aLength, float aRadius,
        std::span<const CircleObstacle> aObstacles, const RectTraversalBounds* aBounds = nullptr);
}
