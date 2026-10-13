#pragma once

#include "../Controller.h"

class BulletSystem;
struct CombatSettings;
class ActorManager;

// Chase and shoot are shared by all three AI systems.
// Each robot has its own instance so their enemy references and shot timers differ.
class CombatController final : public Controller
{
public:
    CombatController(BulletSystem& aBulletSystem, const CombatSettings& aSettings);
    void SetEnemy(Actor& aEnemy);
    // U06 chooses once before the robots move. Actor order breaks equal ties.
    void SelectNearestEnemy(const Actor& aActor, ActorManager& aActors);
    bool HasDetectedEnemy(const Actor& aActor) const;
    const Actor* GetEnemy() const;
    float GetTimeUntilNextShot() const;
    // The world advances this timer; only an active Combat action fires a shot.
    void UpdateCooldown(float aDeltaTime);
    void ResetCooldown();
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;

private:
    BulletSystem& myBulletSystem;
    const CombatSettings& mySettings;
    Actor* myEnemy = nullptr;
    float myTimeUntilNextShot = 0.f; // Seconds; zero means ready to fire.
};
