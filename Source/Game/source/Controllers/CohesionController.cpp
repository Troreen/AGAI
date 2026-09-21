#include "CohesionController.h"
#include "../Actor.h"
#include "ControllerUtils.h"

CohesionController::CohesionController(const FlockingSettings& aSettings)
    : Controller({ .useContainment = false })
    , mySettings(aSettings)
{
}

CommonUtilities::Vector2f CohesionController::GetDesiredVelocity(const Actor& aActor, std::span<const Actor* const> aNeighbours) const
{
    if (aNeighbours.empty())
    {
        // No neighbours means no cohesion force, rather than a request to stop.
        return aActor.GetVelocity();
    }

    CommonUtilities::Vector2f target = {};
    for (const Actor* neighbour : aNeighbours)
    {
        target += neighbour->GetPosition();
    }
    target /= static_cast<float>(aNeighbours.size());
    return ControllerUtils::SeekDesiredVelocity(aActor, target);
}

float CohesionController::GetBehaviorWeight() const { return mySettings.cohesionWeight; }

bool CohesionController::NeedsNeighbours() const
{
    return true;
}
