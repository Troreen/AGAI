#pragma once

#include <Vector2.hpp>

// The steering system only depends on this interface. Later projects can replace
// RectTraversalBounds with a tile map, polygon, or nav-mesh implementation.
class ITraversalBounds
{
public:
    virtual ~ITraversalBounds() = default;

    // Projects a point into navigable space after applying the supplied inset.
    virtual CommonUtilities::Vector2f GetNearestValidPoint(
        const CommonUtilities::Vector2f& aPosition,
        float aInset) const = 0;

    // Returns a vector pointing back towards valid space. A zero vector means
    // the predicted position is valid.
    virtual CommonUtilities::Vector2f GetRecoveryDirection(
        const CommonUtilities::Vector2f& aCurrentPosition,
        const CommonUtilities::Vector2f& aPredictedPosition,
        float aInset) const = 0;
};

class RectTraversalBounds final : public ITraversalBounds
{
public:
    RectTraversalBounds(const CommonUtilities::Vector2f& aMin, const CommonUtilities::Vector2f& aMax);

    CommonUtilities::Vector2f GetNearestValidPoint(
        const CommonUtilities::Vector2f& aPosition,
        float aInset) const override;

    CommonUtilities::Vector2f GetRecoveryDirection(
        const CommonUtilities::Vector2f& aCurrentPosition,
        const CommonUtilities::Vector2f& aPredictedPosition,
        float aInset) const override;

private:
    CommonUtilities::Vector2f myMin;
    CommonUtilities::Vector2f myMax;
};
