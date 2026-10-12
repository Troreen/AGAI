#include "../Source/Game/source/Actors/Actor.h"
#include "../Source/Game/source/Controllers/Navigation/PathFollowingController.h"
#include "../Source/Game/source/Navigation/NavMesh.h"
#include <cstdio>

int main(int argc, char** argv)
{
    NavMesh mesh;
    const bool loaded = mesh.LoadFbx(argc > 1 ? argv[1] : "../EngineAssets/Models/NavMesh_Models/navmesh_debug.fbx", {100.f, 80.f}, {1180.f, 640.f});
    if (!loaded) return 2;
    unsigned int routes = 0;
    unsigned int failures = 0;
    const auto& triangles = mesh.GetTriangles();
    for (float delta : {1.f / 30.f, 1.f / 60.f, 1.f / 120.f})
    {
        for (unsigned int sample = 0; sample < 64; ++sample)
        {
            const auto start = triangles[(sample * 17) % triangles.size()].centre;
            const auto goal = triangles[(sample * 31 + 11) % triangles.size()].centre;
            const auto path = mesh.FindPath(start, goal);
            if (!path.Succeeded()) continue;
            Actor actor;
            actor.SetPosition(start);
            actor.SetMaxSpeed(200.f);
            actor.SetMaxForce(1200.f);
            actor.SetMass(0.15f);
            auto controller = std::make_unique<PathFollowingController>(mesh);
            auto* follower = controller.get();
            follower->SetPath(path.smoothPoints);
            actor.SetController(std::move(controller));
            bool stayedOnMesh = true;
            for (int frame = 0; frame < static_cast<int>(60.f / delta) && !follower->IsFinished(); ++frame)
            {
                auto before = actor.GetPosition();
                if (argc > 2)
                {
                    actor.Update(delta); // Reproduce the old combined snap/movement check.
                }
                else
                {
                    actor.CalculateSteering(delta);
                    before = actor.GetPosition();
                    actor.UpdateMovement(delta);
                }
                const auto proposed = actor.GetPosition();
                const auto constrained = mesh.ConstrainMovement(before, proposed);
                const auto followInfo = follower->GetDebugInfo();
                if (constrained != proposed || (argc <= 2 && followInfo.hasTarget && !mesh.CanTraverseSegment(proposed, followInfo.targetPosition)))
                {
                    if (argc > 2)
                        actor.SetPosition(constrained);
                    else
                    {
                        actor.SetPosition(before);
                        const auto safeStep = before + follower->GetDesiredVelocity(actor) * delta;
                        actor.SetPosition(mesh.ConstrainMovement(before, safeStep));
                    }
                    actor.Stop();
                }
                stayedOnMesh &= mesh.FindTriangle(actor.GetPosition()) >= 0;
            }
            ++routes;
            if (!follower->IsFinished() || !stayedOnMesh || actor.GetPosition().Distance(goal) > 1.5f)
            {
                ++failures;
                std::printf("FAIL sample %u dt %.5f points %zu at %.2f,%.2f goal %.2f,%.2f\n",
                    sample, delta, path.smoothPoints.size(), actor.GetPosition().x, actor.GetPosition().y, goal.x, goal.y);
                const auto target = follower->GetDebugInfo().targetPosition;
                std::printf("Target %.3f,%.3f speed %.3f traversable %d\n", target.x, target.y, actor.GetVelocity().Length(), mesh.CanTraverseSegment(actor.GetPosition(), target));
                for (const auto& point : path.smoothPoints)
                    std::printf("Waypoint %.3f,%.3f distance %.3f traversable %d\n", point.x, point.y, actor.GetPosition().Distance(point), mesh.CanTraverseSegment(actor.GetPosition(), point));
            }
        }
    }
    std::printf("%u routes, %u failures\n", routes, failures);
    return failures ? 1 : 0;
}
