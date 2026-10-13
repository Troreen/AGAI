#include "CombatController.h"
#include "../ControllerUtils.h"
#include "../../Actors/Actor.h"
#include "../../Actors/ActorManager.h"
#include "../../Worlds/05_RobotWars/BulletSystem.h"
#include "../../Worlds/05_RobotWars/CombatSettings.h"
#include <algorithm>

CombatController::CombatController(BulletSystem& aBulletSystem, const CombatSettings& aSettings)
    : myBulletSystem(aBulletSystem), mySettings(aSettings)
{}

void CombatController::SetEnemy(Actor& aEnemy)
{
    myEnemy = &aEnemy;
}

void CombatController::SelectNearestEnemy(const Actor& aActor, ActorManager& aActors)
{
    myEnemy = nullptr;
    if (!aActor.IsAlive()) return;
    float nearestDistanceSquared = 0.f;
    for (std::size_t index = 0; index < aActors.GetActorCount(); ++index)
    {
        Actor& candidate = aActors.GetActor(index);
        if (&candidate == &aActor || !candidate.IsAlive()) continue;
        const float distanceSquared = aActor.GetPosition().DistanceSqr(candidate.GetPosition());
        if (!myEnemy || distanceSquared < nearestDistanceSquared)
        {
            myEnemy = &candidate;
            nearestDistanceSquared = distanceSquared;
        }
    }
}

bool CombatController::HasDetectedEnemy(const Actor& aActor) const
{
    // An empty rectangular arena only needs a distance check for vision.
    if (!myEnemy || !aActor.IsAlive() || !myEnemy->IsAlive())
    {
        return false;
    }

    const float detectionRadiusSquared = mySettings.detectionRadius * mySettings.detectionRadius;
    // Squared distances give the same range check without taking a square root.
    return aActor.GetPosition().DistanceSqr(myEnemy->GetPosition()) <= detectionRadiusSquared;
}

void CombatController::UpdateCooldown(float aDeltaTime)
{
    // The world ticks this even while the AI chooses healing or wandering.
    myTimeUntilNextShot = (std::max)(0.f, myTimeUntilNextShot - aDeltaTime);
}

void CombatController::ResetCooldown()
{
    myTimeUntilNextShot = 0.f;
}

const Actor* CombatController::GetEnemy() const { return myEnemy; }
float CombatController::GetTimeUntilNextShot() const { return myTimeUntilNextShot; }

void CombatController::Update(Actor& aActor, float /*aDeltaTime*/)
{
    // The AI calls this only when Combat is selected. Aim at the enemy's
    // current position, then wait one firing interval before the next shot.
    if (HasDetectedEnemy(aActor) && myTimeUntilNextShot <= 0.f)
    {
        myBulletSystem.Fire(aActor, *myEnemy, mySettings);
        myTimeUntilNextShot = mySettings.fireInterval;
    }
}

CommonUtilities::Vector2f CombatController::GetDesiredVelocity(const Actor& aActor) const
{
    if (!HasDetectedEnemy(aActor))
    {
        return {};
    }

    // Chase a point just short of the enemy. Arrive slows the approach so the
    // robot can shoot from nearby without trying to stand inside its target.
    const CommonUtilities::Vector2f toEnemy = myEnemy->GetPosition() - aActor.GetPosition();
    if (toEnemy.LengthSqr() <= mySettings.stoppingDistance * mySettings.stoppingDistance)
    {
        // Asking for zero velocity makes steering brake, rather than snapping to a stop.
        return {};
    }

    const CommonUtilities::Vector2f chasePosition = myEnemy->GetPosition() - toEnemy.GetNormalized() * mySettings.stoppingDistance;
    return ControllerUtils::ArriveDesiredVelocity(aActor, chasePosition, mySettings.chaseSlowingDistance);
}
