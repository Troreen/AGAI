#pragma once

#include "../Controller.h"

class WanderController;
class ArriveController;
class CombatController;

enum class RobotState
{
    Wander,
    Heal,
    Combat
};

// Keep one state until one of its outgoing conditions asks for a change.
class StateMachineController final : public Controller
{
public:
    StateMachineController(WanderController& aWander, ArriveController& aHeal, CombatController& aCombat);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    RobotState GetState() const;
    void Reset();

private:
    Controller& GetStateController() const;
    WanderController& myWander;
    ArriveController& myHeal;
    CombatController& myCombat;
    RobotState myState = RobotState::Wander;
};
