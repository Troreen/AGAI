#include "../source/ObstacleAvoidance.h"
#include "../source/Controllers/Controller.h"
#include "../source/Interfaces/TraversalBounds.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <numbers>

int main()
{
    assert(!ControllerData{}.useContainment);
    using CommonUtilities::Vector2f;
    const Vector2f origin{ 0.f, 0.f };
    const Vector2f forward{ 1.f, 0.f };
    const std::array<CircleObstacle, 1> ahead = { CircleObstacle{ { 10.f, 0.f }, 2.f } };

    const auto clear = ObstacleAvoidance::FindDirection(origin, forward, 20.f, 1.f, {});
    assert(!clear.forwardBlocked && clear.testedRayCount == 1 && clear.foundClearDirection);
    assert(clear.direction == forward);

    const auto hit = ObstacleAvoidance::CastRay(origin, forward, 20.f, 1.f, ahead);
    assert(hit.blocked && std::abs(hit.distance - 7.f) < 0.0001f);
    assert(!ObstacleAvoidance::CastRay(origin, forward, 6.f, 1.f, ahead).blocked);
    assert(!ObstacleAvoidance::CastRay(origin, -forward, 20.f, 1.f, ahead).blocked);

    const std::array<CircleObstacle, 2> multiple = {
        CircleObstacle{ { 15.f, 0.f }, 2.f }, CircleObstacle{ { 8.f, 0.f }, 2.f } };
    assert(std::abs(ObstacleAvoidance::CastRay(origin, forward, 20.f, 1.f, multiple).distance - 5.f) < 0.0001f);

    const std::array<CircleObstacle, 1> tangent = { CircleObstacle{ { 10.f, 3.f }, 2.f } };
    assert(ObstacleAvoidance::CastRay(origin, forward, 20.f, 1.f, tangent).blocked);

    const auto avoided = ObstacleAvoidance::FindDirection(origin, forward, 20.f, 1.f, ahead);
    assert(avoided.forwardBlocked && avoided.foundClearDirection && avoided.testedRayCount > 1);
    assert(!ObstacleAvoidance::CastRay(origin, avoided.direction, 20.f, 1.f, ahead).blocked);
    // Sorting the golden-angle samples should choose a small turn, not an unnecessary U-turn.
    assert(avoided.direction.x > 0.8f);
    assert(std::abs(avoided.direction.Length() - 1.f) < 0.0001f);

    const std::array<CircleObstacle, 1> above = { CircleObstacle{ { 0.f, 10.f }, 2.f } };
    const auto rotated = ObstacleAvoidance::FindDirection(origin, { 0.f, 1.f }, 20.f, 1.f, above);
    assert(rotated.forwardBlocked && rotated.foundClearDirection);
    assert(rotated.direction.y > 0.8f);

    // An overlapping fish must be able to leave, but not go deeper into the circle.
    assert(!ObstacleAvoidance::CastRay({ 12.f, 0.f }, forward, 20.f, 1.f, ahead).blocked);
    assert(ObstacleAvoidance::CastRay({ 12.f, 0.f }, -forward, 20.f, 1.f, ahead).blocked);

    std::array<CircleObstacle, 8> enclosure;
    for (std::size_t index = 0; index < enclosure.size(); ++index)
    {
        const float angle = static_cast<float>(index) * std::numbers::pi_v<float> / 4.f;
        enclosure[index] = { { std::cos(angle) * 6.f, std::sin(angle) * 6.f }, 5.f };
    }
    const auto trapped = ObstacleAvoidance::FindDirection(origin, forward, 20.f, 0.f, enclosure);
    assert(trapped.forwardBlocked && !trapped.foundClearDirection);
    assert(trapped.testedRayCount == ObstacleAvoidanceResult::rayCount);

    const auto stopped = ObstacleAvoidance::FindDirection(origin, {}, 20.f, 1.f, ahead);
    assert(stopped.forwardBlocked && stopped.foundClearDirection);
    assert(std::isfinite(stopped.direction.x) && std::isfinite(stopped.direction.y));
    const RectTraversalBounds bounds({ -10.f, -10.f }, { 10.f, 10.f });
    for (const Vector2f direction : std::array<Vector2f, 4>{ forward, -forward, Vector2f{ 0.f, 1.f }, Vector2f{ 0.f, -1.f } })
    {
        const auto wall = ObstacleAvoidance::CastRay(origin, direction, 20.f, 1.f, {}, &bounds);
        assert(wall.blocked && std::abs(wall.distance - 9.f) < 0.0001f);
        assert(!ObstacleAvoidance::CastRay(origin, direction, 8.f, 1.f, {}, &bounds).blocked);
    }
    const auto diagonal = Vector2f{ 1.f, 1.f }.GetNormalized();
    const auto corner = ObstacleAvoidance::CastRay(origin, diagonal, 20.f, 1.f, {}, &bounds);
    assert(corner.blocked && std::abs(corner.distance - std::sqrt(162.f)) < 0.0001f);
    assert(ObstacleAvoidance::CastRay({ 11.f, 0.f }, forward, 5.f, 1.f, {}, &bounds).blocked);
    assert(!ObstacleAvoidance::CastRay({ 11.f, 0.f }, -forward, 5.f, 1.f, {}, &bounds).blocked);
    const auto wallAvoidance = ObstacleAvoidance::FindDirection({ 8.f, 0.f }, forward, 5.f, 1.f, {}, &bounds);
    assert(wallAvoidance.forwardBlocked && wallAvoidance.foundClearDirection);
    assert(!ObstacleAvoidance::CastRay({ 8.f, 0.f }, wallAvoidance.direction, 5.f, 1.f, {}, &bounds).blocked);
    const auto nearerCircle = ObstacleAvoidance::CastRay(origin, forward, 20.f, 1.f, ahead, &bounds);
    assert(nearerCircle.blocked && std::abs(nearerCircle.distance - 7.f) < 0.0001f);
    const auto nearerWall = ObstacleAvoidance::CastRay(origin, forward, 20.f, 1.f,
        std::span<const CircleObstacle>(multiple).first(1), &bounds);
    assert(nearerWall.blocked && std::abs(nearerWall.distance - 9.f) < 0.0001f);
    assert(ObstacleAvoidance::CastRay(origin, forward, 20.f, 11.f, {}, &bounds).blocked);
    std::cout << "Obstacle avoidance geometry and direction tests passed.\n";
}
