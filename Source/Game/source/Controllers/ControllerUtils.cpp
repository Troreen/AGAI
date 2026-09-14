#include "ControllerUtils.h"
#include "../Actor.h"

#include <algorithm>

CommonUtilities::Vector2f ControllerUtils::SeekDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aTarget)
{
    const CommonUtilities::Vector2f toTarget = aTarget - aActor.GetPosition();
    const float distanceToTarget = toTarget.Length();
    if (distanceToTarget <= 0.0001f)
        return {};

    return (toTarget / distanceToTarget) * aActor.GetMaxSpeed();
}

CommonUtilities::Vector2f ControllerUtils::FleeDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aPositionToFlee)
{
    return SeekDesiredVelocity(aActor, aActor.GetPosition() + (aActor.GetPosition() - aPositionToFlee));
}

CommonUtilities::Vector2f ControllerUtils::SteerTowards(const Actor& aActor, const CommonUtilities::Vector2f& aDesiredVelocity)
{
    return aDesiredVelocity - aActor.GetVelocity();
}

CommonUtilities::Vector2f ControllerUtils::ArriveDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aTarget, float aSlowDownDistance)
{
    const CommonUtilities::Vector2f toTarget = aTarget - aActor.GetPosition();
    const float distance = toTarget.Length();
    if (distance <= 0.0001f)
    {
        return {};
    }

    const float safeSlowDownDistance = (std::max)(aSlowDownDistance, 0.0001f);
    const float desiredSpeed = aActor.GetMaxSpeed() * (std::min)(distance / safeSlowDownDistance, 1.f);
    return toTarget.GetNormalized() * desiredSpeed;
}

