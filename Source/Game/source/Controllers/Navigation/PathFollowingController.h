#pragma once

#include "../Controller.h"
#include <vector>

class NavMesh;

// Follows the funnel's segments using existing seek/arrive steering and Actor movement.
class PathFollowingController final : public Controller
{
public:
    explicit PathFollowingController(const NavMesh& aNavMesh);
    void SetPath(const std::vector<CommonUtilities::Vector2f>& someWaypoints);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    ControllerDebugInfo GetDebugInfo() const override;
    bool IsFinished() const;
    const std::vector<CommonUtilities::Vector2f>& GetPath() const;

private:
    const NavMesh& myNavMesh;
    std::vector<CommonUtilities::Vector2f> myWaypoints;
    // The waypoint we still need to reach; waypoint 0 is usually the starting position.
    std::size_t myNextWaypointIndex = 1;
    // This may be ahead of the current waypoint when the shortcut is walkable.
    CommonUtilities::Vector2f mySteeringTarget;
    // Slow down when aiming at the final destination.
    bool myShouldSlowDown = false;
    void ChooseFollowTarget(const CommonUtilities::Vector2f& anActorPosition);
};
