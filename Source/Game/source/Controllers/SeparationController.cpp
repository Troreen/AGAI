#include "SeparationController.h"
#include "../Actor.h"
#include "ControllerUtils.h"

namespace
{
    constexpr float zeroDistanceSqr = 0.0001f;
}

SeparationController::SeparationController(const FlockingSettings& aSettings)
    : Controller({ .useContainment = false })
    , mySettings(aSettings)
{
}

CommonUtilities::Vector2f SeparationController::GetDesiredVelocity(const Actor& aActor) const
{
    if (mySettings.avoidanceRadius <= 0.f)
    {
        return {};
    }

    CommonUtilities::Vector2f away = {};
    for (const Actor* neighbour : aActor.GetNeighbours())
    {
        const CommonUtilities::Vector2f offset = aActor.GetPosition() - neighbour->GetPosition();
        const float distanceSqr = offset.LengthSqr();
        const float avoidanceRadiusSqr = mySettings.avoidanceRadius * mySettings.avoidanceRadius;
        if (distanceSqr <= zeroDistanceSqr || distanceSqr >= avoidanceRadiusSqr)
        {
            continue;
        }

        const float distance = offset.Length();
        const float closeness = (mySettings.avoidanceRadius - distance) / mySettings.avoidanceRadius;
        away += offset.GetNormalized() * closeness;
    }

    if (away.LengthSqr() <= zeroDistanceSqr)
    {
        return {};
    }
    return ControllerUtils::FleeDesiredVelocity(aActor, aActor.GetPosition() - away);
}

float SeparationController::GetBehaviorWeight() const { return mySettings.separationWeight; }
