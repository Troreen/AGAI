#pragma once

#include <tge/math/vector.h>
#include <algorithm>
#include <cmath>
#include <vector>

// Deliberately independent of Actor/Boid.  The same sampled-ray algorithm is
// available to both dimensions; callers only provide position and velocity.
template <typename Vector>
struct SphericalObstacle
{
    Vector center{};
    float radius = 1.f;
};

namespace ObstacleAvoidance
{
    template <typename Vector>
    inline bool HitsSphere(const Vector& origin, const Vector& direction,
        float length, const SphericalObstacle<Vector>& obstacle, float clearance)
    {
        const Vector toObstacle = obstacle.center - origin;
        const float projection = toObstacle.Dot(direction);
        if (projection <= 0.f || projection > length)
            return false;

        const Vector closest = origin + direction * projection;
        const float radius = obstacle.radius + clearance;
        return (obstacle.center - closest).LengthSqr() <= radius * radius;
    }

    template <typename Vector>
    inline Vector SelectDirection(const Vector& position, const Vector& desiredDirection,
        float lookAhead, float clearance, const std::vector<SphericalObstacle<Vector>>& obstacles,
        const std::vector<Vector>& candidates)
    {
        if (desiredDirection.LengthSqr() <= 0.0001f)
            return {};
        const Vector desired = desiredDirection.GetNormalized();

        bool blocked = false;
        for (const auto& obstacle : obstacles)
            blocked |= HitsSphere(position, desired, lookAhead, obstacle, clearance);
        if (!blocked)
            return desired;

        // Candidates are ordered by increasing angular deviation, matching
        // Seb Lague's incremental ray search: keep the first safe direction.
        for (const Vector& candidate : candidates)
        {
            const Vector direction = candidate.GetNormalized();
            bool candidateBlocked = false;
            for (const auto& obstacle : obstacles)
                candidateBlocked |= HitsSphere(position, direction, lookAhead, obstacle, clearance);
            if (!candidateBlocked)
                return direction;
        }
        return desired * -1.f;
    }

    inline Tga::Vector2f Avoid2D(const Tga::Vector2f& position, const Tga::Vector2f& desired,
        float lookAhead, float clearance, const std::vector<SphericalObstacle<Tga::Vector2f>>& obstacles,
        int samples = 8)
    {
        std::vector<Tga::Vector2f> candidates;
        candidates.reserve(samples * 2);
        const float base = std::atan2(desired.y, desired.x);
        for (int i = 1; i <= samples; ++i)
        {
            const float angle = (3.14159265f * 0.5f) * i / samples;
            candidates.push_back({ std::cos(base + angle), std::sin(base + angle) });
            candidates.push_back({ std::cos(base - angle), std::sin(base - angle) });
        }
        return SelectDirection(position, desired, lookAhead, clearance, obstacles, candidates);
    }

    inline Tga::Vector3f Avoid3D(const Tga::Vector3f& position, const Tga::Vector3f& desired,
        float lookAhead, float clearance, const std::vector<SphericalObstacle<Tga::Vector3f>>& obstacles,
        int samples = 5)
    {
        const Tga::Vector3f forward = desired.GetNormalized();
        Tga::Vector3f up = std::abs(forward.Dot(Tga::Vector3f::Up)) > 0.95f
            ? Tga::Vector3f::Right : Tga::Vector3f::Up;
        const Tga::Vector3f right = forward.Cross(up).GetNormalized();
        up = right.Cross(forward).GetNormalized();

        std::vector<Tga::Vector3f> candidates;
        candidates.reserve(samples * 4);
        for (int i = 1; i <= samples; ++i)
        {
            const float angle = (3.14159265f * 0.5f) * i / samples;
            const float c = std::cos(angle), s = std::sin(angle);
            candidates.push_back((forward * c + right * s).GetNormalized());
            candidates.push_back((forward * c - right * s).GetNormalized());
            candidates.push_back((forward * c + up * s).GetNormalized());
            candidates.push_back((forward * c - up * s).GetNormalized());
        }
        return SelectDirection(position, desired, lookAhead, clearance, obstacles, candidates);
    }
}
