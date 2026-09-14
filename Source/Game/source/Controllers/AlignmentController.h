#pragma once

#include "Controller.h"
#include "../FlockingSettings.h"

class AlignmentController final : public Controller
{
public:
    explicit AlignmentController(const FlockingSettings& aSettings);
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    float GetBehaviorWeight() const override;

private:
    const FlockingSettings& mySettings;
};
