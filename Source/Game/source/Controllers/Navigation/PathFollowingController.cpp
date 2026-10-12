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

void PathFollowingController::SetPath(const std::vector<Vector2f>& someWaypoints)
{
    // Keep the path's corners as they are. Usually point 0 is our starting
    // position, so point 1 is the first waypoint to reach.
    myWaypoints = someWaypoints;
    myNextWaypointIndex = myWaypoints.size() == 1 ? 0 : 1;
    mySteeringTarget = myWaypoints.empty() ? Vector2f{} : myWaypoints.front();
    myShouldSlowDown = true;
}

bool PathFollowingController::IsFinished() const
{
    // There is nothing left to follow once the waypoint index is past the end.
    return myNextWaypointIndex >= myWaypoints.size();
}

const std::vector<Vector2f>& PathFollowingController::GetPath() const
{
    // Let the world draw the same waypoints the companion is following.
    return myWaypoints;
}

void PathFollowingController::Update(Actor& aActor, float aDeltaTime)
{
    // Finish reachable waypoints without losing velocity. Select the next
    // movement target this frame; only the destination stops the Actor.
    while (!IsFinished())
    {
        const bool isDestination = myNextWaypointIndex + 1 == myWaypoints.size();
        // Allow intermediate waypoints within one frame of travel, so we do not run past them.
        const float waypointReachDistance = isDestination ? arrivalDistance
            : (std::max)(arrivalDistance, aActor.GetMaxSpeed() * aDeltaTime);
        if (aActor.GetPosition().Distance(myWaypoints[myNextWaypointIndex]) > waypointReachDistance ||
            !myNavMesh.CanTraverseSegment(aActor.GetPosition(), myWaypoints[myNextWaypointIndex]))
        {
            break;
        }
        // Finish this reachable waypoint exactly, then check whether the next one is also close.
        aActor.SetPosition(myWaypoints[myNextWaypointIndex]);
        ++myNextWaypointIndex;
    }
    if (IsFinished())
    {
        aActor.Stop();
        return;
    }
    ChooseFollowTarget(aActor.GetPosition());
}

void PathFollowingController::ChooseFollowTarget(const Vector2f& anActorPosition)
{
    // Start with a safe fallback: steer towards the current waypoint.
    // We will aim farther ahead only if there is a clear way to get there.
    mySteeringTarget = myWaypoints[myNextWaypointIndex];
    myShouldSlowDown = myNextWaypointIndex + 1 == myWaypoints.size();
    // With only one point, there is no path segment to follow. Just arrive there.
    if (myNextWaypointIndex == 0)
    {
        return;
    }

    // Find the closest spot on the part of the path we still have left.
    // This tells us how far we have actually moved along it; aiming ahead
    // does not mean we have already reached that part of the path.
    Vector2f targetPositionAlongPath = myWaypoints[myNextWaypointIndex - 1];
    float nearestDistanceSquared = std::numeric_limits<float>::max();
    for (std::size_t waypointIndex = myNextWaypointIndex; waypointIndex < myWaypoints.size(); ++waypointIndex)
    {
        const Vector2f segmentStart = myWaypoints[waypointIndex - 1];
        const Vector2f segmentVector = myWaypoints[waypointIndex] - segmentStart;
        const float segmentLengthSquared = segmentVector.LengthSqr();
        // The fraction tells us where we are along this segment: 0 is its start,
        // 1 is its end. Clamp keeps the closest spot between those endpoints.
        // A repeated waypoint has no length, so use its starting point.
        const float fractionAlongSegment = segmentLengthSquared > 0.f ? std::clamp((anActorPosition - segmentStart).Dot(segmentVector) / segmentLengthSquared, 0.f, 1.f) : 0.f;
        const Vector2f closestPositionOnSegment = segmentStart + segmentVector * fractionAlongSegment;
        const float distanceSquared = anActorPosition.DistanceSqr(closestPositionOnSegment);
        // Accept a closer spot only if both it and the segment's endpoint
        // are reachable. Otherwise we could skip a corner around a wall.
        if (distanceSquared <= nearestDistanceSquared && myNavMesh.CanTraverseSegment(anActorPosition, closestPositionOnSegment) && myNavMesh.CanTraverseSegment(anActorPosition, myWaypoints[waypointIndex]))
        {
            nearestDistanceSquared = distanceSquared;
            targetPositionAlongPath = closestPositionOnSegment;
            myNextWaypointIndex = waypointIndex;
        }
    }
    mySteeringTarget = myWaypoints[myNextWaypointIndex];
    myShouldSlowDown = myNextWaypointIndex + 1 == myWaypoints.size();

    // From that closest spot, move the target lookAheadDistance pixels forward along the path.
    // If a segment is too short, spend the leftover distance on the next one.
    float remainingLookAheadDistance = lookAheadDistance;
    std::size_t targetWaypointIndex = myNextWaypointIndex;
    while (targetWaypointIndex < myWaypoints.size())
    {
        const Vector2f offsetToSegmentEnd = myWaypoints[targetWaypointIndex] - targetPositionAlongPath;
        const float distanceToSegmentEnd = offsetToSegmentEnd.Length();
        if (distanceToSegmentEnd > remainingLookAheadDistance)
        {
            // The target fits on this segment. Move partway along it and finish.
            targetPositionAlongPath += offsetToSegmentEnd * (remainingLookAheadDistance / distanceToSegmentEnd);
            break;
        }
        targetPositionAlongPath = myWaypoints[targetWaypointIndex];
        remainingLookAheadDistance -= distanceToSegmentEnd;
        // Stop at the destination if the path ends before we use all lookAheadDistance pixels.
        if (targetWaypointIndex + 1 == myWaypoints.size())
        {
            break;
        }
        ++targetWaypointIndex;
    }

    // We steer directly towards the target, so the whole shortcut must be
    // walkable. If it crosses a wall, keep the fallback waypoint chosen above.
    if (myNavMesh.CanTraverseSegment(anActorPosition, targetPositionAlongPath))
    {
        mySteeringTarget = targetPositionAlongPath;
        // Slow down only when the look-ahead target reaches the destination.
        myShouldSlowDown = targetPositionAlongPath == myWaypoints.back();
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
    return myShouldSlowDown
        ? ControllerUtils::ArriveDesiredVelocity(aActor, mySteeringTarget, lookAheadDistance)
        : ControllerUtils::SeekDesiredVelocity(aActor, mySteeringTarget);
}

ControllerDebugInfo PathFollowingController::GetDebugInfo() const
{
    ControllerDebugInfo debugInfo;
    if (!IsFinished())
    {
        // Show the point we are steering towards, which may be between waypoints.
        debugInfo.hasTarget = true;
        debugInfo.targetPosition = mySteeringTarget;
    }
    return debugInfo;
}
