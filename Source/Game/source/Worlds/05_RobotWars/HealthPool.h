#pragma once

#include <Math/Vector2.hpp>

// The world heals living robots whose centres are inside this circle.
// This data describes the pool; UpdateHealthPool applies the healing each frame.
struct HealthPool
{
    CommonUtilities::Vector2f position = {};
    float radius = 70.f;
    float healingPerSecond = 10.f; // Multiply by frame time to get the HP gained.
};
