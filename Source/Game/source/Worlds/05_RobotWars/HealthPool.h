#pragma once

#include <Math/Vector2.hpp>

struct HealthPool
{
    CommonUtilities::Vector2f position = {};
    float radius = 70.f;
    float healingPerSecond = 10.f;
};
