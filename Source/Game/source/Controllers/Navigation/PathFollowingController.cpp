#include "PathFollowingController.h"
#include "../ControllerUtils.h"
#include "../../Actors/Actor.h"
#include "../../Navigation/NavMesh.h"
#include <algorithm>

using CommonUtilities::Vector2f;

namespace
{
constexpr float cornerClearance = 30.f;
constexpr float lookAheadDistance = 45.f;
constexpr float gentleTurnDot = 0.866f; // About 30 degrees; gentler turns keep full speed.
constexpr float minimumTurnSpeed = 0.65f; // Sharp turns keep at least 65% of the normal speed.
}

PathFollowingController::PathFollowingController(const NavMesh& aNavMesh) : myNavMesh(aNavMesh)
{
}

void PathFollowingController::SetPath(const std::vector<Vector2f>& somePoints)
{
    myPoints = somePoints;
    // The shortest funnel path can hug a wall. Give each bend some turning
    // room, but only when both connecting segments still stay on the navmesh.
    for (std::size_t index = 1; index + 1 < myPoints.size(); ++index)
    {
        const Vector2f incoming = (myPoints[index] - myPoints[index - 1]).GetNormalized();
        const Vector2f outgoing = (myPoints[index + 1] - myPoints[index]).GetNormalized();
        const float clearance = (std::min)(cornerClearance,
            (std::min)(myPoints[index].Distance(myPoints[index - 1]),
                       myPoints[index].Distance(myPoints[index + 1])) * 0.25f);
        const Vector2f easedCorner = myPoints[index] + (incoming - outgoing).GetNormalized() * clearance;
        if (myNavMesh.CanTraverseSegment(myPoints[index - 1], easedCorner) &&
            myNavMesh.CanTraverseSegment(easedCorner, myPoints[index + 1]))
        {
            myPoints[index] = easedCorner;
        }
    }
    myNextPoint = 1;
    myTurning = false;
}

bool PathFollowingController::IsFinished() const
{
    return myNextPoint >= myPoints.size();
}

const std::vector<Vector2f>& PathFollowingController::GetPath() const
{
    return myPoints;
}

Vector2f PathFollowingController::GetNextSegmentTarget() const
{
    const Vector2f corner = myPoints[myNextPoint];
    const Vector2f next = myPoints[myNextPoint + 1];
    const float distance = (std::min)(lookAheadDistance, corner.Distance(next));
    return corner + (next - corner).GetNormalized() * distance;
}

void PathFollowingController::Update(Actor& aActor, float)
{
    myTurning = false;
    if (IsFinished())
    {
        aActor.Stop();
        return;
    }
    const Vector2f corner = myPoints[myNextPoint];
    if (myNextPoint + 1 < myPoints.size() &&
        aActor.GetPosition().Distance(corner) <= lookAheadDistance &&
        myNavMesh.CanTraverseSegment(aActor.GetPosition(), GetNextSegmentTarget()))
    {
        // Carry our velocity through a bend when there is a clear way around it.
        ++myNextPoint;
        return;
    }
    if (aActor.GetPosition().Distance(corner) <= 1.5f &&
        myNavMesh.CanTraverseSegment(aActor.GetPosition(), corner))
    {
        // Tight corners still need an exact arrival. Only stop here when we
        // could not safely steer onto the next segment, or when reaching the goal.
        aActor.SetPosition(corner);
        aActor.Stop();
        ++myNextPoint;
        myTurning = true;
    }
}

Vector2f PathFollowingController::GetDesiredVelocity(const Actor& aActor) const
{
    if (IsFinished() || myTurning)
    {
        return {};
    }
    const Vector2f start = myPoints[myNextPoint - 1];
    const Vector2f end = myPoints[myNextPoint];
    const Vector2f segment = end - start;
    const float length = segment.Length();
    if (length <= 0.001f)
    {
        return {};
    }

    // Project our future position onto the active segment, then seek a little
    // farther along it. Cross a bend only when the whole shortcut is walkable.
    const Vector2f future = aActor.GetPosition() + aActor.GetVelocity() * 0.2f;
    const float along = std::clamp((future - start).Dot(segment) / length, 0.f, length);
    const float lookAhead = (std::min)(along + lookAheadDistance, length);
    Vector2f target = start + segment * (lookAhead / length);
    const bool hasNextSegment = myNextPoint + 1 < myPoints.size();
    bool canRoundCorner = false;
    if (hasNextSegment && length - along <= lookAheadDistance)
    {
        const Vector2f nextTarget = GetNextSegmentTarget();
        canRoundCorner = myNavMesh.CanTraverseSegment(aActor.GetPosition(), nextTarget);
        if (canRoundCorner)
        {
            target = nextTarget;
        }
    }
    if (!myNavMesh.CanTraverseSegment(aActor.GetPosition(), target))
    {
        target = end;
    }
    // Ordinary bends are passing points, not destinations. Brake only for the
    // goal or a corner that has no walkable shortcut onto its next segment.
    const Vector2f direction = (target - aActor.GetPosition()).GetNormalized();
    float speed = !hasNextSegment || !canRoundCorner
        ? ControllerUtils::ArriveDesiredVelocity(aActor, end, lookAheadDistance).Length()
        : aActor.GetMaxSpeed();
    if (aActor.GetVelocity().LengthSqr() > 1.f)
    {
        // Ease off while our movement direction turns towards the look-ahead
        // target. As we line up again, the normal speed comes back gradually.
        const float alignment = aActor.GetVelocity().GetNormalized().Dot(direction);
        const float sharpness = std::clamp((gentleTurnDot - alignment) / gentleTurnDot, 0.f, 1.f);
        speed *= 1.f - sharpness * (1.f - minimumTurnSpeed);
    }
    return direction * speed;
}

ControllerDebugInfo PathFollowingController::GetDebugInfo() const
{
    ControllerDebugInfo info;
    if (!IsFinished())
    {
        info.hasTarget = true;
        info.targetPosition = myPoints[myNextPoint];
    }
    return info;
}
