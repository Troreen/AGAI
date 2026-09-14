#include "AlignmentController.h"
#include "../Actor.h"

AlignmentController::AlignmentController(const FlockingSettings& aSettings)
    : Controller({ .useContainment = false })
    , mySettings(aSettings)
{
}

CommonUtilities::Vector2f AlignmentController::GetDesiredVelocity(const Actor& aActor) const
{
    const std::vector<const Actor*>& neighbours = aActor.GetNeighbours();
    if (neighbours.empty())
    {
        return {};
    }

    CommonUtilities::Vector2f averageVelocity = {};
    for (const Actor* neighbour : neighbours)
    {
        averageVelocity += neighbour->GetVelocity();
    }

    return averageVelocity / static_cast<float>(neighbours.size());
}

float AlignmentController::GetBehaviorWeight() const { return mySettings.alignmentWeight; }
