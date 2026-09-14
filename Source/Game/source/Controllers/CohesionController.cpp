#include "CohesionController.h"
#include "../Actor.h"
#include "ControllerUtils.h"

CohesionController::CohesionController(const FlockingSettings& aSettings)
    : Controller({ .useContainment = false })
    , mySettings(aSettings)
{
}

CommonUtilities::Vector2f CohesionController::GetDesiredVelocity(const Actor& aActor) const
{
    const std::vector<const Actor*>& neighbours = aActor.GetNeighbours();
    if (neighbours.empty())
    {
        return {};
    }

    CommonUtilities::Vector2f target = {};
    for (const Actor* neighbour : neighbours)
    {
        target += neighbour->GetPosition();
    }
    target /= static_cast<float>(neighbours.size());
    return ControllerUtils::SeekDesiredVelocity(aActor, target);
}

float CohesionController::GetBehaviorWeight() const { return mySettings.cohesionWeight; }
