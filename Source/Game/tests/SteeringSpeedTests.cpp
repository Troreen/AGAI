#include "../source/Actor.h"
#include "../source/Controllers/AlignmentController.h"
#include "../source/Controllers/CohesionController.h"
#include "../source/Controllers/SeparationController.h"
#include "../source/Controllers/ObstacleAvoidanceController.h"
#include "../source/Controllers/WanderController.h"
#include "../source/Interfaces/TraversalBounds.h"

#include <cassert>
#include <iostream>

int main()
{
    FlockingSettings settings;
    Actor actor;
    actor.SetRadius(6.f);
    actor.SetMaxSpeed(1000.f);
    actor.AddSteeringForce({ 100.f, 0.f });
    actor.UpdateMovement(1.f);

    AlignmentController alignment(settings);
    CohesionController cohesion(settings);
    SeparationController separation(settings);
    assert(alignment.GetSteeringForce(actor).LengthSqr() == 0.f);
    assert(cohesion.GetSteeringForce(actor).LengthSqr() == 0.f);
    assert(separation.GetSteeringForce(actor).LengthSqr() == 0.f);

    Actor neighbour;
    const std::array<const Actor*, 1> neighbours = { &neighbour };
    settings.avoidanceRadius = 10.f;
    assert(separation.GetSteeringForce(actor, neighbours).LengthSqr() == 0.f);
    settings.avoidanceRadius = 0.f;
    assert(separation.GetSteeringForce(actor, neighbours).LengthSqr() == 0.f);

    ObstacleAvoidanceSettings avoidanceSettings;
    const std::vector<CircleObstacle> obstacles;
    auto bounds = std::make_shared<RectTraversalBounds>(CommonUtilities::Vector2f{ -100.f, -100.f },
        CommonUtilities::Vector2f{ 100.f, 100.f });
    ObstacleAvoidanceController avoidance(avoidanceSettings, obstacles, bounds);
    assert(avoidance.EvaluateAvoidance(actor).forwardBlocked);
    assert(avoidance.GetSteeringForce(actor).Dot(actor.GetVelocity()) <= 0.f);
    for (int frame = 0; frame < 120; ++frame)
    {
        const float previousSpeed = actor.GetVelocity().Length();
        actor.AddSteeringForce(avoidance.GetSteeringForce(actor));
        actor.UpdateMovement(1.f / 60.f);
        assert(actor.GetVelocity().Length() <= previousSpeed + 0.001f);
    }

    // Inactive flocking must not make a lone wandering fish slower than wander alone.
    Actor wanderOnly;
    Actor withFlocking;
    WanderControllerData wanderData;
    wanderData.behaviorWeight = 0.35f;
    wanderOnly.AddController(std::make_unique<WanderController>(wanderData));
    withFlocking.AddController(std::make_unique<WanderController>(wanderData));
    withFlocking.AddController(std::make_unique<AlignmentController>(settings));
    withFlocking.AddController(std::make_unique<CohesionController>(settings));
    withFlocking.AddController(std::make_unique<SeparationController>(settings));
    for (int frame = 0; frame < 300; ++frame)
    {
        wanderOnly.Update(1.f / 60.f);
        withFlocking.Update(1.f / 60.f);
        assert(wanderOnly.GetVelocity().DistanceSqr(withFlocking.GetVelocity()) < 0.0001f);
    }
    std::cout << "Steering speed regression tests passed.\n";
}
