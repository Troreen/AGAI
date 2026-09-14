#include "SteeringDebugRenderer.h"
#include "Actor.h"
#include "Controllers/Controller.h"

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
    debugDrawer.DrawCircle(aActor.GetPosition().ToTga(), aActor.GetRadius(), Tga::Color(1, 1, 1, 0.8f));
    if (containment.isEnabled)
    {
        debugDrawer.DrawCircle(aActor.GetPosition().ToTga(), aActor.GetRadius() + containment.boundaryClearance, Tga::Color(0.5f, 0.5f, 0.5f, 0.45f));
        const Tga::Color predictionColor = containment.needsCorrection ? Tga::Color(1, 0.4f, 0, 1) : Tga::Color(0, 1, 1, 1);
        debugDrawer.DrawArrow(aActor.GetPosition().ToTga(), containment.predictedPosition.ToTga(), predictionColor, 5.f);
        debugDrawer.DrawCircle(containment.predictedPosition.ToTga(), 6.f, predictionColor);

        if (containment.needsCorrection)
        {
            const Tga::Color recoveryColor(1, 0, 1, 1);
            debugDrawer.DrawCircle(containment.nearestValidPoint.ToTga(), 8.f, recoveryColor);
            debugDrawer.DrawArrow(containment.predictedPosition.ToTga(), containment.nearestValidPoint.ToTga(), recoveryColor, 5.f);
        }
    }

    if (info.hasTarget)
    {
        const Tga::Color targetColor(1, 1, 0, 1);
        debugDrawer.DrawCircle(info.targetPosition.ToTga(), 12.f, targetColor);
        debugDrawer.DrawArrow(aActor.GetPosition().ToTga(), (aActor.GetPosition() + aController.GetDesiredVelocity(aActor)).ToTga(), Tga::Color(0, 1, 0, 1), 10.f);
        if (info.hasSlowingRadius)
            debugDrawer.DrawCircle(info.targetPosition.ToTga(), info.slowingRadius, targetColor);
    }

    if (info.wanderCircleRadius > 0.f)
    {
        const Tga::Color wanderColor(0, 0.5f, 1, 1);
        const CommonUtilities::Vector2f wanderTarget = info.wanderCircleCenter + CommonUtilities::Vector2f(
            cosf(info.wanderTargetAngle), sinf(info.wanderTargetAngle)) * info.wanderCircleRadius;

        debugDrawer.DrawCircle(info.wanderCircleCenter.ToTga(), info.wanderCircleRadius, wanderColor);
        debugDrawer.DrawArrow(aActor.GetPosition().ToTga(), info.wanderCircleCenter.ToTga(), wanderColor, 5.f);
        debugDrawer.DrawCircle(wanderTarget.ToTga(), 10.f, Tga::Color(1, 1, 0, 1));
        debugDrawer.DrawArrow(info.wanderCircleCenter.ToTga(), wanderTarget.ToTga(), Tga::Color(1, 1, 0, 1), 5.f);
        debugDrawer.DrawArrow(aActor.GetPosition().ToTga(), (aActor.GetPosition() + aController.GetDesiredVelocity(aActor)).ToTga(), Tga::Color(0, 1, 0, 1), 10.f);
    }

    debugDrawer.DrawArrow(aActor.GetPosition().ToTga(), (aActor.GetPosition() + aActor.GetPreviousSteeringForce()).ToTga(), Tga::Color(1, 0, 0, 1), 10.f);
}
#else
void SteeringDebugRenderer::Draw(const Actor&, const Controller&) const {}
#endif
