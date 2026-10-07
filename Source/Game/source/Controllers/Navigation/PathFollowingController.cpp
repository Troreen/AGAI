#include "PathFollowingController.h"
#include "../ControllerUtils.h"
#include "../../Actors/Actor.h"
#include "../../Navigation/NavMesh.h"
#include <algorithm>
#include <limits>

using CommonUtilities::Vector2f;

namespace
{
// Aim this many pixels ahead along the path.
constexpr float lookAheadDistance = 45.f;
// Treat a waypoint as reached when we are this close to it.
constexpr float arrivalDistance = 1.5f;
}

PathFollowingController::PathFollowingController(const NavMesh& aNavMesh) : myNavMesh(aNavMesh)
{
}

void PathFollowingController::SetPath(const std::vector<Vector2f>& somePoints)
{
    // Keep the path's corners as they are. Usually point 0 is our starting
    // position, so point 1 is the first waypoint to reach.
    myPoints = somePoints;
    myNextPoint = myPoints.size() == 1 ? 0 : 1;
    myFollowTarget = myPoints.empty() ? Vector2f{} : myPoints.front();
    myArriving = true;
}

bool PathFollowingController::IsFinished() const
{
    // There is nothing left to follow once the waypoint index is past the end.
    return myNextPoint >= myPoints.size();
}

const std::vector<Vector2f>& PathFollowingController::GetPath() const
{
    return myPoints;
}

void PathFollowingController::Update(Actor& aActor, float)
{
    // When we are close enough, finish the last small step to the waypoint.
    // Check that step is walkable, then stop so incoming velocity cannot
    // carry us through a wall when we turn at a tight corner.
    if (!IsFinished() && aActor.GetPosition().Distance(myPoints[myNextPoint]) <= arrivalDistance && myNavMesh.CanTraverseSegment(aActor.GetPosition(), myPoints[myNextPoint]))
    {
        aActor.SetPosition(myPoints[myNextPoint]);
        aActor.Stop();
        ++myNextPoint;
        // Stay here for this frame. Choose the next movement target next frame.
        myFollowTarget = aActor.GetPosition();
        myArriving = true;
        return;
    }
    if (IsFinished())
    {
        aActor.Stop();
        return;
    }
    ChooseFollowTarget(aActor.GetPosition());
}

void PathFollowingController::ChooseFollowTarget(const Vector2f& aPosition)
{
    // Start with a safe fallback: approach the current waypoint and slow down.
    // We will aim farther ahead only if there is a clear way to get there.
    myFollowTarget = myPoints[myNextPoint];
    myArriving = true;
    // With only one point, there is no path segment to follow. Just arrive there.
    if (myNextPoint == 0)
    {
        return;
    }

    // Find the closest spot on the part of the path we still have left.
    // This tells us how far we have actually moved along it; aiming ahead
    // does not mean we have already reached that part of the path.
    Vector2f position = myPoints[myNextPoint - 1];
    float nearestDistance = std::numeric_limits<float>::max();
    for (std::size_t index = myNextPoint; index < myPoints.size(); ++index)
    {
        const Vector2f start = myPoints[index - 1];
        const Vector2f segment = myPoints[index] - start;
        const float lengthSqr = segment.LengthSqr();
        // "amount" says where we are along this segment: 0 is its start,
        // 1 is its end. Clamp keeps the closest spot between those endpoints.
        // A repeated waypoint has no length, so use its starting point.
        const float amount = lengthSqr > 0.f ? std::clamp((aPosition - start).Dot(segment) / lengthSqr, 0.f, 1.f) : 0.f;
        const Vector2f projection = start + segment * amount;
        const float distance = aPosition.DistanceSqr(projection);
        // Accept a closer spot only if both it and the segment's endpoint
        // are reachable. Otherwise we could skip a corner around a wall.
        if (distance <= nearestDistance && myNavMesh.CanTraverseSegment(aPosition, projection) && myNavMesh.CanTraverseSegment(aPosition, myPoints[index]))
        {
            nearestDistance = distance;
            position = projection;
            myNextPoint = index;
        }
    }
    myFollowTarget = myPoints[myNextPoint];

    // From that closest spot, move the target lookAheadDistance pixels forward along the path.
    // If a segment is too short, spend the leftover distance on the next one.
    float remaining = lookAheadDistance;
    std::size_t targetSegment = myNextPoint;
    while (targetSegment < myPoints.size())
    {
        const Vector2f toEnd = myPoints[targetSegment] - position;
        const float distance = toEnd.Length();
        if (distance > remaining)
        {
            // The target fits on this segment. Move partway along it and finish.
            position += toEnd * (remaining / distance);
            break;
        }
        position = myPoints[targetSegment];
        remaining -= distance;
        // Stop at the destination if the path ends before we use all lookAheadDistance pixels.
        if (targetSegment + 1 == myPoints.size())
        {
            break;
        }
        ++targetSegment;
    }

    // We steer directly towards the target, so the whole shortcut must be
    // walkable. If it crosses a wall, keep the fallback waypoint chosen above.
    if (myNavMesh.CanTraverseSegment(aPosition, position))
    {
        myFollowTarget = position;
        // Slow down when following the final segment towards the destination.
        myArriving = targetSegment + 1 == myPoints.size();
    }
}

Vector2f PathFollowingController::GetDesiredVelocity(const Actor& aActor) const
{
    if (IsFinished())
    {
        return {};
    }
    // Arrive reduces speed as we get close; Seek moves at normal speed.
    // Actor turns this desired velocity into steering and actual movement.
    return myArriving
        ? ControllerUtils::ArriveDesiredVelocity(aActor, myFollowTarget, lookAheadDistance)
        : ControllerUtils::SeekDesiredVelocity(aActor, myFollowTarget);
}

ControllerDebugInfo PathFollowingController::GetDebugInfo() const
{
    ControllerDebugInfo info;
    if (!IsFinished())
    {
        // Show the point we are steering towards, which may be between waypoints.
        info.hasTarget = true;
        info.targetPosition = myFollowTarget;
    }
    return info;
}
