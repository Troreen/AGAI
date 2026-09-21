#include "AlignmentController.h"
#include "../Actor.h"

AlignmentController::AlignmentController(const FlockingSettings& aSettings)
    : Controller({ .useContainment = false })
    , mySettings(aSettings)
{
}

CommonUtilities::Vector2f AlignmentController::GetDesiredVelocity(const Actor& aActor, std::span<const Actor* const> aNeighbours) const
{
    if (aNeighbours.empty())
    {
        // No neighbours means no alignment force, rather than a request to stop.
        return aActor.GetVelocity();
    }

    CommonUtilities::Vector2f averageVelocity = {};
    for (const Actor* neighbour : aNeighbours)
    {
        averageVelocity += neighbour->GetVelocity();
    }

    return averageVelocity / static_cast<float>(aNeighbours.size());
}

float AlignmentController::GetBehaviorWeight() const { return mySettings.alignmentWeight; }

bool AlignmentController::NeedsNeighbours() const
{
    return true;
}
