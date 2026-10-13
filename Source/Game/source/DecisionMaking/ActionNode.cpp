#include "ActionNode.h"
#include "../Controllers/Controller.h"

ActionNode::ActionNode(Controller& aMovementController)
    : myMovementController(aMovementController)
{}

ActionNode* ActionNode::MakeDecision(const Actor& /*aActor*/)
{
    // Reaching a leaf means the tree has found the action for this frame.
    return this;
}

void ActionNode::Update(Actor& aActor, float aDeltaTime)
{
    myMovementController.Update(aActor, aDeltaTime);
}

CommonUtilities::Vector2f ActionNode::GetDesiredVelocity(const Actor& aActor) const
{
    return myMovementController.GetDesiredVelocity(aActor);
}
