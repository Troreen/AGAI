#include "BulletSystem.h"
#include "CombatSettings.h"
#include "../../Actors/Actor.h"
#include "../../Interfaces/TraversalBounds.h"
#include <algorithm>

void BulletSystem::Fire(const Actor& aShooter, Actor& aTarget, const CombatSettings& aSettings)
{
    if (!aShooter.IsAlive() || !aTarget.IsAlive())
    {
        return;
    }

    const CommonUtilities::Vector2f toTarget = aTarget.GetPosition() - aShooter.GetPosition();
    if (toTarget.LengthSqr() <= 0.0001f)
    {
        return;
    }

    // Aim once when firing. The bullet then flies straight instead of homing.
    Bullet bullet;
    bullet.position = aShooter.GetPosition();
    bullet.velocity = toTarget.GetNormalized() * aSettings.bulletSpeed;
    bullet.damage = aSettings.bulletDamage;
    bullet.radius = aSettings.bulletRadius;
    bullet.remainingLifetime = aSettings.bulletLifetime;
    bullet.target = &aTarget;
    myBullets.push_back(bullet);
}

void BulletSystem::Update(float aDeltaTime, const RectTraversalBounds& aArenaBounds)
{
    for (Bullet& bullet : myBullets)
    {
        if (!bullet.target->IsAlive())
        {
            bullet.remainingLifetime = 0.f;
            continue;
        }

        const CommonUtilities::Vector2f previousPosition = bullet.position;
        // Only travel for the part of this frame that the bullet is still alive.
        const float travelTime = (std::min)(aDeltaTime, bullet.remainingLifetime);
        bullet.position += bullet.velocity * travelTime;
        bullet.remainingLifetime -= aDeltaTime;

        if (HitsTarget(bullet, previousPosition))
        {
            bullet.target->TakeDamage(bullet.damage);
            bullet.remainingLifetime = 0.f;
        }
        else
        {
            const CommonUtilities::Vector2f minimum = aArenaBounds.GetMin();
            const CommonUtilities::Vector2f maximum = aArenaBounds.GetMax();
            if (bullet.position.x < minimum.x || bullet.position.x > maximum.x ||
                bullet.position.y < minimum.y || bullet.position.y > maximum.y)
            {
                bullet.remainingLifetime = 0.f;
            }
        }
    }

    // A consumed bullet cannot damage the target again on another frame.
    std::erase_if(myBullets, [](const Bullet& aBullet) { return aBullet.remainingLifetime <= 0.f; });
}

bool BulletSystem::HitsTarget(const Bullet& aBullet, const CommonUtilities::Vector2f& aPreviousPosition) const
{
    // Check the whole travelled segment so a fast bullet cannot skip over a robot.
    const CommonUtilities::Vector2f travelledSegment = aBullet.position - aPreviousPosition;
    const float segmentLengthSquared = travelledSegment.LengthSqr();
    float closestPointFraction = 0.f;
    if (segmentLengthSquared > 0.0001f)
    {
        const CommonUtilities::Vector2f toTarget = aBullet.target->GetPosition() - aPreviousPosition;
        closestPointFraction = std::clamp(toTarget.Dot(travelledSegment) / segmentLengthSquared, 0.f, 1.f);
    }

    const CommonUtilities::Vector2f closestPoint = aPreviousPosition + travelledSegment * closestPointFraction;
    const float hitRadius = aBullet.target->GetRadius() + aBullet.radius;
    return closestPoint.DistanceSqr(aBullet.target->GetPosition()) <= hitRadius * hitRadius;
}

void BulletSystem::Clear()
{
    myBullets.clear();
}

const std::vector<Bullet>& BulletSystem::GetBullets() const
{
    return myBullets;
}
