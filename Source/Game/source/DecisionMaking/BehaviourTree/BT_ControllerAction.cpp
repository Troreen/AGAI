#include "BT_ControllerAction.h"
#include "../../Controllers/Controller.h"

BT_ControllerAction::BT_ControllerAction(Controller& aController)
    : myController(aController)
{}

BT_NodeResult BT_ControllerAction::Update(Actor& aActor, float aDeltaTime, BehaviourTree&)
{
    myController.Update(aActor, aDeltaTime);
    // These behaviours keep going until the tree chooses another branch.
    return BT_NodeResult::Running;
}

CommonUtilities::Vector2f BT_ControllerAction::GetDesiredVelocity(const Actor& aActor) const
{
    return myController.GetDesiredVelocity(aActor);
}
