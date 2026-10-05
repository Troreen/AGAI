#include "PathFollowingController.h"
#include "ControllerUtils.h"
#include "../Actor.h"
#include "../Navigation/NavMesh.h"
#include <algorithm>

using CommonUtilities::Vector2f;

PathFollowingController::PathFollowingController(const NavMesh& aNavMesh) : myNavMesh(aNavMesh)
{
}

void PathFollowingController::SetPath(const std::vector<Vector2f>& somePoints)
{
    myPoints = somePoints;
    myNextPoint = 1;
    myTurning = false;
}

bool PathFollowingController::IsFinished() const
{
    return myNextPoint >= myPoints.size();
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
    if (aActor.GetPosition().Distance(corner) <= 1.5f &&
        myNavMesh.CanTraverseSegment(aActor.GetPosition(), corner))
    {
        // A funnel can touch a wall corner exactly. Arrive first, then turn from
        // that corner, so inertia cannot round the turn through an unwalkable hole.
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
    // farther along it. We never skip a corner just because another segment is near.
    const Vector2f future = aActor.GetPosition() + aActor.GetVelocity() * 0.2f;
    const float along = std::clamp((future - start).Dot(segment) / length, 0.f, length);
    const float lookAhead = (std::min)(along + 45.f, length);
    Vector2f target = start + segment * (lookAhead / length);
    if (!myNavMesh.CanTraverseSegment(aActor.GetPosition(), target))
    {
        target = end;
    }
    // Slow for the actual corner/end, not for the moving look-ahead point.
    const Vector2f direction = (target - aActor.GetPosition()).GetNormalized();
    const float speed = ControllerUtils::ArriveDesiredVelocity(aActor, end, 45.f).Length();
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
