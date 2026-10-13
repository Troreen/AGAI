#pragma once

// Both AI implementations will use these same combat values.
struct CombatSettings
{
    float detectionRadius = 480.f;
    float stoppingDistance = 120.f;
    // Begin braking early enough to keep inertia from carrying us into the enemy.
    float chaseSlowingDistance = 240.f;
    float fireInterval = 0.5f;
    float bulletSpeed = 600.f;
    // A shot can finish a robot leaving Heal at 40 HP, avoiding a shared-pool loop.
    float bulletDamage = 45.f;
    float bulletRadius = 5.f;
    float bulletLifetime = 2.f;
};
