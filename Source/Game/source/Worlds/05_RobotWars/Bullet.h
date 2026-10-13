#pragma once

#include <Math/Vector2.hpp>

class Actor;

struct Bullet
{
    CommonUtilities::Vector2f position = {};
    CommonUtilities::Vector2f velocity = {};
    float damage = 0.f;
    float radius = 0.f;
    float remainingLifetime = 0.f;
    // Actors stay owned by ActorManager while their bullets are in flight.
    Actor* target = nullptr;
};
