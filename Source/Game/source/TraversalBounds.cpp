#include "TraversalBounds.h"

#include <algorithm>

RectTraversalBounds::RectTraversalBounds(const Tga::Vector2f& aMin, const Tga::Vector2f& aMax)
    : myMin(aMin)
    , myMax(aMax)
{
}

Tga::Vector2f RectTraversalBounds::GetNearestValidPoint(
    const Tga::Vector2f& aPosition,
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

Tga::Vector2f RectTraversalBounds::GetRecoveryDirection(
    const Tga::Vector2f& aCurrentPosition,
    const Tga::Vector2f& aPredictedPosition,
    float aInset) const
{
    const Tga::Vector2f nearestValidPoint = GetNearestValidPoint(aPredictedPosition, aInset);

    if ((nearestValidPoint - aPredictedPosition).LengthSqr() <= 0.0001f)
        return {};

    // Aim from the current position, so an actor already outside gets a clear
    // inward direction instead of merely cancelling its forward velocity.
    return nearestValidPoint - aCurrentPosition;
}

RectTraversalBounds3D::RectTraversalBounds3D(const Tga::Vector3f& aMin, const Tga::Vector3f& aMax)
    : myMin(aMin), myMax(aMax)
{
}

Tga::Vector3f RectTraversalBounds3D::GetNearestValidPoint(const Tga::Vector3f& p, float inset) const
{
    const float minX = (std::min)(myMin.x + inset, myMax.x), maxX = (std::max)(myMax.x - inset, myMin.x);
    const float minY = (std::min)(myMin.y + inset, myMax.y), maxY = (std::max)(myMax.y - inset, myMin.y);
    const float minZ = (std::min)(myMin.z + inset, myMax.z), maxZ = (std::max)(myMax.z - inset, myMin.z);
    return { std::clamp(p.x, minX, maxX), std::clamp(p.y, minY, maxY), std::clamp(p.z, minZ, maxZ) };
}

Tga::Vector3f RectTraversalBounds3D::GetRecoveryDirection(const Tga::Vector3f& current,
    const Tga::Vector3f& predicted, float inset) const
{
    const Tga::Vector3f nearest = GetNearestValidPoint(predicted, inset);
    if ((nearest - predicted).LengthSqr() <= 0.0001f) return {};
    return nearest - current;
}
