#pragma once

struct FlockingSettings
{
    // How far a boid can see, in world units. Only boid inside this radius are neighbours.
    float perceptionRadius = 100.f;
    // Start pushing away inside this distance. Only detected neighbours are considered.
    float avoidanceRadius = 55.f;
    // How strongly boid match their neighbours' average velocity; zero turns alignment off.
    float alignmentWeight = 2.5f;
    // How strongly boid move towards their neighbours' average position; zero turns cohesion off.
    float cohesionWeight = .5f;
    // How strongly boid avoid close neighbours; zero turns separation off.
    float separationWeight = .5f;
};
