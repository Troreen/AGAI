#include "TraversalBounds.h"

#include <algorithm>

ITraversalBounds::~ITraversalBounds() = default;

const CommonUtilities::Vector2f& RectTraversalBounds::GetMin() const
{
    return myMin;
}

const CommonUtilities::Vector2f& RectTraversalBounds::GetMax() const
{
    return myMax;
}

RectTraversalBounds::RectTraversalBounds(const CommonUtilities::Vector2f& aMin, const CommonUtilities::Vector2f& aMax)
    : myMin(aMin)
    , myMax(aMax)
{
}

CommonUtilities::Vector2f RectTraversalBounds::GetNearestValidPoint(
    const CommonUtilities::Vector2f& aPosition,
    float aInset) const
{
    const float minX = (std::min)(myMin.x + aInset, myMax.x);
    const float maxX = (std::max)(myMax.x - aInset, myMin.x);
    const float minY = (std::min)(myMin.y + aInset, myMax.y);
    const float maxY = (std::max)(myMax.y - aInset, myMin.y);

    return
    {
        std::clamp(aPosition.x, minX, maxX),
        std::clamp(aPosition.y, minY, maxY)
    };
}

CommonUtilities::Vector2f RectTraversalBounds::GetRecoveryDirection(
    const CommonUtilities::Vector2f& aCurrentPosition,
    const CommonUtilities::Vector2f& aPredictedPosition,
    float aInset) const
{
    const CommonUtilities::Vector2f nearestValidPoint = GetNearestValidPoint(aPredictedPosition, aInset);

    if ((nearestValidPoint - aPredictedPosition).LengthSqr() <= 0.0001f)
        return {};

    // Aim from the current position, so an actor already outside gets a clear
    // inward direction instead of merely cancelling its forward velocity.
    return nearestValidPoint - aCurrentPosition;
}
