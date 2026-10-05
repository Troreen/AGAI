#pragma once

#include "../Controller.h"
#include <vector>

class NavMesh;

// Follows the funnel's segments using existing seek/arrive steering and Actor movement.
class PathFollowingController final : public Controller
{
public:
    explicit PathFollowingController(const NavMesh& aNavMesh);
    void SetPath(const std::vector<CommonUtilities::Vector2f>& somePoints);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    ControllerDebugInfo GetDebugInfo() const override;
    bool IsFinished() const;
    const std::vector<CommonUtilities::Vector2f>& GetPath() const;

private:
    const NavMesh& myNavMesh;
    std::vector<CommonUtilities::Vector2f> myPoints;
    std::size_t myNextPoint = 1;
    bool myTurning = false;
    CommonUtilities::Vector2f GetNextSegmentTarget() const;
};
