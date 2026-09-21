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

CommonUtilities::Vector2f SeparationController::GetDesiredVelocity(const Actor& aActor, std::span<const Actor* const> aNeighbours) const
{
    if (mySettings.avoidanceRadius <= 0.f)
    {
        return aActor.GetVelocity();
    }

    CommonUtilities::Vector2f away = {};
    for (const Actor* neighbour : aNeighbours)
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
        // Nothing to avoid: preserve motion so this controller contributes zero force.
        return aActor.GetVelocity();
    }
    return ControllerUtils::FleeDesiredVelocity(aActor, aActor.GetPosition() - away);
}

float SeparationController::GetBehaviorWeight() const { return mySettings.separationWeight; }

bool SeparationController::NeedsNeighbours() const
{
    return true;
}
