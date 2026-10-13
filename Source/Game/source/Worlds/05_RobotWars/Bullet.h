#pragma once

#include <Math/Vector2.hpp>

class Actor;

// One straight-flying shot. Its velocity is set when fired and never follows
// the target, so a moving robot can dodge it.
struct Bullet
{
    CommonUtilities::Vector2f position = {};
    CommonUtilities::Vector2f velocity = {};
    float damage = 0.f;
    float radius = 0.f;
    float remainingLifetime = 0.f; // Seconds left; zero also marks a consumed shot.
    // This is the enemy that can be hit, not a homing destination.
    // ActorManager keeps actors alive in memory even when their HP reaches zero.
    Actor* target = nullptr;
};
