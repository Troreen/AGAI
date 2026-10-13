#include "ActionNode.h"
#include "../Controllers/Controller.h"

ActionNode::ActionNode(Controller& aActionController)
    : myActionController(aActionController)
{}

ActionNode* ActionNode::MakeDecision(const Actor& /*aActor*/)
{
    // Reaching a leaf means the tree has found the action for this frame.
    return this;
}

void ActionNode::Update(Actor& aActor, float aDeltaTime)
{
    // Let the selected controller do its work. Actor will ask for its steering next.
    myActionController.Update(aActor, aDeltaTime);
}

CommonUtilities::Vector2f ActionNode::GetDesiredVelocity(const Actor& aActor) const
{
    return myActionController.GetDesiredVelocity(aActor);
}
