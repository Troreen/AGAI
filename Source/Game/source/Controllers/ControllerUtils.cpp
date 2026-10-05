#include "ControllerUtils.h"
#include "../Actor.h"

#include <algorithm>

// --- Head straight towards a position at full speed ---
CommonUtilities::Vector2f ControllerUtils::SeekDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aTarget)
{
    const CommonUtilities::Vector2f toTarget = aTarget - aActor.GetPosition();
    const float distanceToTarget = toTarget.Length();
    if (distanceToTarget <= 0.0001f)
    {
        return {};
    }

    return (toTarget / distanceToTarget) * aActor.GetMaxSpeed();
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
// The player and all four guards use this same arrival rule.
CommonUtilities::Vector2f ControllerUtils::ArriveDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aTarget, float aSlowDownDistance)
{
    const CommonUtilities::Vector2f toTarget = aTarget - aActor.GetPosition();
    const float distance = toTarget.Length();
    if (distance <= 0.0001f)
    {
        return {};
    }

    const float safeSlowDownDistance = (std::max)(aSlowDownDistance, 0.0001f);
    // Far away means full speed. Inside the slowing distance, closer means slower.
    const float desiredSpeed = aActor.GetMaxSpeed() * (std::min)(distance / safeSlowDownDistance, 1.f);
    return toTarget.GetNormalized() * desiredSpeed;
}
