#pragma once

#include <Vector2.hpp>

class Actor;

namespace ControllerUtils
{
    CommonUtilities::Vector2f SeekDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aTarget);
    CommonUtilities::Vector2f FleeDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aPositionToFlee);
    CommonUtilities::Vector2f SteerTowards(const Actor& aActor, const CommonUtilities::Vector2f& aDesiredVelocity);

    CommonUtilities::Vector2f ArriveDesiredVelocity(const Actor& aActor, const CommonUtilities::Vector2f& aTarget, float aSlowDownDistance);
}
