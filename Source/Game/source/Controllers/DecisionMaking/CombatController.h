#pragma once

#include "../Controller.h"

class BulletSystem;
struct CombatSettings;

// Chase and shoot are actions shared by the Decision Tree and later State Machine.
class CombatController final : public Controller
{
public:
    CombatController(BulletSystem& aBulletSystem, const CombatSettings& aSettings);
    void SetEnemy(Actor& aEnemy);
    bool HasDetectedEnemy(const Actor& aActor) const;
    void UpdateCooldown(float aDeltaTime);
    void ResetCooldown();
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;

private:
    BulletSystem& myBulletSystem;
    const CombatSettings& mySettings;
    Actor* myEnemy = nullptr;
    float myTimeUntilNextShot = 0.f;
};
