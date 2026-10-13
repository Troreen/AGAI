#include "StateMachineController.h"
#include "CombatController.h"
#include "../Steering/WanderController.h"
#include "../Steering/ArriveController.h"
#include "../../Actors/Actor.h"

StateMachineController::StateMachineController(WanderController& aWanderController, ArriveController& aHealingController, CombatController& aCombatController)
    : myWanderController(aWanderController), myHealingController(aHealingController), myCombatController(aCombatController)
{}

void StateMachineController::Update(Actor& aActor, float aDeltaTime)
{
    // Each state checks its own exits. Wounded always takes priority over combat.
    switch (myCurrentState)
    {
    case RobotState::Wander:
        if (aActor.IsWounded())
            myCurrentState = RobotState::Heal;
        else if (myCombatController.HasDetectedEnemy(aActor))
            myCurrentState = RobotState::Combat;
        break;
    case RobotState::Heal:
        // Keep seeking the pool until HP reaches the wounded threshold.
        // Then return straight to combat if the enemy is still visible.
        if (!aActor.IsWounded())
            myCurrentState = myCombatController.HasDetectedEnemy(aActor) ? RobotState::Combat : RobotState::Wander;
        break;
    case RobotState::Combat:
        if (aActor.IsWounded())
            myCurrentState = RobotState::Heal;
        else if (!myCombatController.HasDetectedEnemy(aActor))
            myCurrentState = RobotState::Wander;
        break;
    }
    // Run the new state in this same frame. Do not run the old combat action
    // after choosing Heal, or the wounded robot could fire one extra shot.
    GetCurrentStateController().Update(aActor, aDeltaTime);
}

CommonUtilities::Vector2f StateMachineController::GetDesiredVelocity(const Actor& aActor) const
{
    // Choosing a state does not move the robot directly. Its action asks for
    // a velocity, and Actor combines that steering with wall avoidance.
    return GetCurrentStateController().GetDesiredVelocity(aActor);
}

Controller& StateMachineController::GetCurrentStateController() const
{
    // Map each state to its existing movement/action controller.
    switch (myCurrentState)
    {
    case RobotState::Heal: return myHealingController;
    case RobotState::Combat: return myCombatController;
    case RobotState::Wander: return myWanderController;
    }
    return myWanderController;
}

RobotState StateMachineController::GetState() const { return myCurrentState; }
void StateMachineController::Reset() { myCurrentState = RobotState::Wander; }
