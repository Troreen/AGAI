#pragma once

#include <tge/math/vector.h>

// The steering system only depends on this interface. Later projects can replace
// RectTraversalBounds with a tile map, polygon, or nav-mesh implementation.
class ITraversalBounds
{
public:
    virtual ~ITraversalBounds() = default;

    // Projects a point into navigable space after applying the supplied inset.
    virtual Tga::Vector2f GetNearestValidPoint(
        const Tga::Vector2f& aPosition,
        float aInset) const = 0;

    // Returns a vector pointing back towards valid space. A zero vector means
    // the predicted position is valid.
    virtual Tga::Vector2f GetRecoveryDirection(
        const Tga::Vector2f& aCurrentPosition,
        const Tga::Vector2f& aPredictedPosition,
        float aInset) const = 0;
};

class RectTraversalBounds final : public ITraversalBounds
{
public:
    RectTraversalBounds(const Tga::Vector2f& aMin, const Tga::Vector2f& aMax);

    Tga::Vector2f GetNearestValidPoint(
        const Tga::Vector2f& aPosition,
        float aInset) const override;

    Tga::Vector2f GetRecoveryDirection(
        const Tga::Vector2f& aCurrentPosition,
        const Tga::Vector2f& aPredictedPosition,
        float aInset) const override;

private:
    Tga::Vector2f myMin;
    Tga::Vector2f myMax;
};
