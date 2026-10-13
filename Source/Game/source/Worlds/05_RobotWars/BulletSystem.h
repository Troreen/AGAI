#pragma once

#include "Bullet.h"
#include <vector>

class Actor;
class RectTraversalBounds;
struct CombatSettings;

// Own the flying bullets and resolve their hits after the actors have moved.
class BulletSystem
{
public:
    void Fire(const Actor& aShooter, Actor& aTarget, const CombatSettings& aSettings);
    void Update(float aDeltaTime, const RectTraversalBounds& aArenaBounds);
    void Clear();
    const std::vector<Bullet>& GetBullets() const;

private:
    bool HitsTarget(const Bullet& aBullet, const CommonUtilities::Vector2f& aPreviousPosition) const;
    std::vector<Bullet> myBullets;
};
