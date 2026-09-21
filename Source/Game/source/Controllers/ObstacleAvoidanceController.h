#pragma once

#include "Controller.h"
#include "../ObstacleAvoidance.h"
#include <vector>

class ObstacleAvoidanceController final : public Controller
{
public:
    // The world owns settings and obstacles; both must outlive this controller.
    ObstacleAvoidanceController(const ObstacleAvoidanceSettings& aSettings,
        const std::vector<CircleObstacle>& aObstacles, std::shared_ptr<const RectTraversalBounds> aBounds);
    CommonUtilities::Vector2f GetSteeringForce(const Actor& aActor,
        std::span<const Actor* const> aNeighbours = {}) const override;
    ObstacleAvoidanceResult EvaluateAvoidance(const Actor& aActor) const;

private:
    const ObstacleAvoidanceSettings& mySettings;
    const std::vector<CircleObstacle>& myObstacles;
    std::shared_ptr<const RectTraversalBounds> myBounds;
};
