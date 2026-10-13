#include "DecisionTreeController.h"
#include "../../DecisionMaking/DecisionTree.h"
#include "../../DecisionMaking/ActionNode.h"

DecisionTreeController::DecisionTreeController(DecisionTree& aDecisionTree)
    : myDecisionTree(aDecisionTree)
{}

void DecisionTreeController::Update(Actor& aActor, float aDeltaTime)
{
    // Reconsider the tree every frame, then update only the chosen movement.
    mySelectedAction = myDecisionTree.MakeDecision(aActor);
    mySelectedAction->Update(aActor, aDeltaTime);
}

CommonUtilities::Vector2f DecisionTreeController::GetDesiredVelocity(const Actor& aActor) const
{
    // Actor updates controllers before asking for steering. Before that first
    // update there is no selected action, so there is no requested movement.
    if (!mySelectedAction)
    {
        return {};
    }

    return mySelectedAction->GetDesiredVelocity(aActor);
}

const ActionNode* DecisionTreeController::GetSelectedAction() const
{
    return mySelectedAction;
}
