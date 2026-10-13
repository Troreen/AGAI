#include "StateMachineController.h"
#include "CombatController.h"
#include "../Steering/WanderController.h"
#include "../Steering/ArriveController.h"
#include "../../Actors/Actor.h"

StateMachineController::StateMachineController(WanderController& aWander, ArriveController& aHeal, CombatController& aCombat)
    : myWander(aWander), myHeal(aHeal), myCombat(aCombat)
{}

void StateMachineController::Update(Actor& aActor, float aDeltaTime)
{
    // Each state checks its own exits. Wounded always takes priority over combat.
    switch (myState)
    {
    case RobotState::Wander:
        if (aActor.IsWounded())
            myState = RobotState::Heal;
        else if (myCombat.HasDetectedEnemy(aActor))
            myState = RobotState::Combat;
        break;
    case RobotState::Heal:
        if (!aActor.IsWounded())
            myState = myCombat.HasDetectedEnemy(aActor) ? RobotState::Combat : RobotState::Wander;
        break;
    case RobotState::Combat:
        if (aActor.IsWounded())
            myState = RobotState::Heal;
        else if (!myCombat.HasDetectedEnemy(aActor))
            myState = RobotState::Wander;
        break;
    }
    GetStateController().Update(aActor, aDeltaTime);
}

CommonUtilities::Vector2f StateMachineController::GetDesiredVelocity(const Actor& aActor) const
{
    return GetStateController().GetDesiredVelocity(aActor);
}

Controller& StateMachineController::GetStateController() const
{
    switch (myState)
    {
    case RobotState::Heal: return myHeal;
    case RobotState::Combat: return myCombat;
    case RobotState::Wander: return myWander;
    }
    return myWander;
}

RobotState StateMachineController::GetState() const { return myState; }
void StateMachineController::Reset() { myState = RobotState::Wander; }
