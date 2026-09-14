#pragma once

struct FlockingSettings
{
    float perceptionRadius = 100.f;
    float avoidanceRadius = 55.f;
    float alignmentWeight = 1.f;
    float cohesionWeight = 1.f;
    float separationWeight = 1.5f;
};
