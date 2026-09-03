#include "SteeringDebugRenderer.h"
#include "Actor.h"
#include "Controller.h"

#include <cmath>

#if !IS_RETAIL_BUILD
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/DebugDrawer.h>

void SteeringDebugRenderer::Draw(const Actor& aActor, const Controller& aController) const
{
    const ControllerDebugInfo info = aController.GetDebugInfo();
    const ContainmentDebugInfo containment = aController.GetContainmentDebugInfo(aActor);
    auto& debugDrawer = Tga::GraphicsEngine::GetInstance()->GetDebugDrawer();

    // White: physical actor radius. Grey: radius plus the desired clearance.
    debugDrawer.DrawCircle(aActor.GetPosition(), aActor.GetRadius(), Tga::Color(1, 1, 1, 0.8f));
    if (containment.isEnabled)
    {
        debugDrawer.DrawCircle(aActor.GetPosition(), aActor.GetRadius() + containment.boundaryClearance, Tga::Color(0.5f, 0.5f, 0.5f, 0.45f));
        const Tga::Color predictionColor = containment.needsCorrection ? Tga::Color(1, 0.4f, 0, 1) : Tga::Color(0, 1, 1, 1);
        debugDrawer.DrawArrow(aActor.GetPosition(), containment.predictedPosition, predictionColor, 5.f);
        debugDrawer.DrawCircle(containment.predictedPosition, 6.f, predictionColor);

        if (containment.needsCorrection)
        {
            const Tga::Color recoveryColor(1, 0, 1, 1);
            debugDrawer.DrawCircle(containment.nearestValidPoint, 8.f, recoveryColor);
            debugDrawer.DrawArrow(containment.predictedPosition, containment.nearestValidPoint, recoveryColor, 5.f);
        }
    }

    if (info.hasTarget)
    {
        const Tga::Color targetColor(1, 1, 0, 1);
        debugDrawer.DrawCircle(info.targetPosition, 12.f, targetColor);
        debugDrawer.DrawArrow(aActor.GetPosition(), aActor.GetPosition() + aController.GetDesiredVelocity(aActor), Tga::Color(0, 1, 0, 1), 10.f);
        if (info.hasSlowingRadius)
            debugDrawer.DrawCircle(info.targetPosition, info.slowingRadius, targetColor);
    }

    if (info.wanderCircleRadius > 0.f)
    {
        const Tga::Color wanderColor(0, 0.5f, 1, 1);
        const Tga::Vector2f wanderTarget = info.wanderCircleCenter + Tga::Vector2f(
            cosf(info.wanderTargetAngle), sinf(info.wanderTargetAngle)) * info.wanderCircleRadius;

        debugDrawer.DrawCircle(info.wanderCircleCenter, info.wanderCircleRadius, wanderColor);
        debugDrawer.DrawArrow(aActor.GetPosition(), info.wanderCircleCenter, wanderColor, 5.f);
        debugDrawer.DrawCircle(wanderTarget, 10.f, Tga::Color(1, 1, 0, 1));
        debugDrawer.DrawArrow(info.wanderCircleCenter, wanderTarget, Tga::Color(1, 1, 0, 1), 5.f);
        debugDrawer.DrawArrow(aActor.GetPosition(), aActor.GetPosition() + aController.GetDesiredVelocity(aActor), Tga::Color(0, 1, 0, 1), 10.f);
    }

    debugDrawer.DrawArrow(aActor.GetPosition(), aActor.GetPosition() + aActor.GetPreviousSteeringForce(), Tga::Color(1, 0, 0, 1), 10.f);
}
#else
void SteeringDebugRenderer::Draw(const Actor&, const Controller&) const {}
#endif
