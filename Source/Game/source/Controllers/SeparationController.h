#pragma once

#include "Controller.h"
#include "../FlockingSettings.h"

class SeparationController final : public Controller
{
public:
    explicit SeparationController(const FlockingSettings& aSettings);
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor, std::span<const Actor* const> aNeighbours) const override;
    bool NeedsNeighbours() const override;
    float GetBehaviorWeight() const override;

private:
    const FlockingSettings& mySettings;
};
