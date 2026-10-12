#include "ControllerUtils.h"
#include "../Actors/Actor.h"

#include <algorithm>

// --- Head straight towards a position at full speed ---
CommonUtilities::Vector2f ControllerUtils::SeekDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aTarget)
{
    const CommonUtilities::Vector2f offsetToTarget = aTarget - aActor.GetPosition();
    const float distanceToTarget = offsetToTarget.Length();
    if (distanceToTarget <= 0.0001f)
    {
        return {};
    }

    // Use the direction towards the target, with the actor's full movement speed.
    return (offsetToTarget / distanceToTarget) * aActor.GetMaxSpeed();
}

// --- Head away from a position instead ---
CommonUtilities::Vector2f ControllerUtils::FleeDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aPositionToFlee)
{
    return SeekDesiredVelocity(aActor, aActor.GetPosition() + (aActor.GetPosition() - aPositionToFlee));
}

// --- Work out the push needed to change our current movement ---
// Desired movement minus current movement tells us what needs to change.
CommonUtilities::Vector2f ControllerUtils::SteerTowards(const Actor& aActor, const CommonUtilities::Vector2f& aDesiredVelocity)
{
    return aDesiredVelocity - aActor.GetVelocity();
}

// --- Head towards a position, but slow down near it ---
// Controllers use this when they should slow down near their destination.
CommonUtilities::Vector2f ControllerUtils::ArriveDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aTarget, float aSlowDownDistance)
{
    const CommonUtilities::Vector2f offsetToTarget = aTarget - aActor.GetPosition();
    const float distanceToTarget = offsetToTarget.Length();
    if (distanceToTarget <= 0.0001f)
    {
        return {};
    }

    // Keep the slowing distance above zero so the speed calculation can divide by it.
    const float safeSlowDownDistance = (std::max)(aSlowDownDistance, 0.0001f);
    // Far away means full speed. Inside the slowing distance, closer means slower.
    const float desiredSpeed = aActor.GetMaxSpeed() * (std::min)(distanceToTarget / safeSlowDownDistance, 1.f);
    return offsetToTarget.GetNormalized() * desiredSpeed;
}
