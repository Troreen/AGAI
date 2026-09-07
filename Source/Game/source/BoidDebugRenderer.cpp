#include "BoidDebugRenderer.h"
#include "Boid.h"

#if !IS_RETAIL_BUILD
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/DebugDrawer.h>

namespace
{
    void DrawBoidHeading(const Boid& aBoid, const Tga::Color& aColor)
    {
        const Tga::Vector2f velocity = aBoid.GetVelocity();
        if (velocity.LengthSqr() <= 0.0001f)
            return;

        constexpr float headingLength = 45.f;
        const Tga::Vector2f heading = velocity.GetNormalized() * headingLength;
        Tga::GraphicsEngine::GetInstance()->GetDebugDrawer().DrawArrow(
            aBoid.GetPosition(), aBoid.GetPosition() + heading, aColor, 6.f);
    }
}

void BoidDebugRenderer::DrawSelected(const Boid& aSelectedBoid) const
{
    auto& debugDrawer = Tga::GraphicsEngine::GetInstance()->GetDebugDrawer();

    // The selected boid is red; its perceived flock is drawn in cyan.
    debugDrawer.DrawCircle(aSelectedBoid.GetPosition(), aSelectedBoid.GetRadius(), Tga::Color(1, 0, 0, 1));
    DrawBoidHeading(aSelectedBoid, Tga::Color(1, 0, 0, 1));

    for (const Boid* flockMember : aSelectedBoid.GetFlockMembers())
    {
        debugDrawer.DrawCircle(flockMember->GetPosition(), flockMember->GetRadius(), Tga::Color(0, 1, 1, 0.6f));
        DrawBoidHeading(*flockMember, Tga::Color(0, 1, 1, 1));
    }
}
#else
void BoidDebugRenderer::DrawSelected(const Boid&) const {}
#endif
