#pragma once

#include "../Controller.h"

class BulletSystem;
struct CombatSettings;

// Chase and shoot are shared by the Decision Tree and State Machine.
// Each robot has its own instance so their enemy references and shot timers differ.
class CombatController final : public Controller
{
public:
    CombatController(BulletSystem& aBulletSystem, const CombatSettings& aSettings);
    void SetEnemy(Actor& aEnemy);
    bool HasDetectedEnemy(const Actor& aActor) const;
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
