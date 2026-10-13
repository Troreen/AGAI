#pragma once

#include "../Controller.h"

class WanderController;
class ArriveController;
class CombatController;

// These are living behaviours. Actor handles death before this controller runs.
enum class RobotState
{
    Wander,
    Heal,
    Combat
};

// Keep one state until one of its outgoing conditions asks for a change.
// The world owns the action controllers; this class only borrows them.
class StateMachineController final : public Controller
{
public:
    StateMachineController(WanderController& aWanderController, ArriveController& aHealingController, CombatController& aCombatController);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    // Read the state used this frame, for the label above the robot.
    RobotState GetState() const;
    void Reset();

private:
    Controller& GetCurrentStateController() const;
    WanderController& myWanderController;
    ArriveController& myHealingController;
    CombatController& myCombatController;
    RobotState myCurrentState = RobotState::Wander;
};
