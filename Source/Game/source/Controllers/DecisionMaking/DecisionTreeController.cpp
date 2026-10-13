#include "DecisionTreeController.h"
#include "../../DecisionMaking/DecisionTree.h"
#include "../../DecisionMaking/ActionNode.h"

DecisionTreeController::DecisionTreeController(DecisionTree& aDecisionTree)
    : myDecisionTree(aDecisionTree)
{}

void DecisionTreeController::Update(Actor& aActor, float aDeltaTime)
{
    // Start at the root every frame. Questions lead to one action leaf, and
    // only that action runs: choosing Heal must not also update Combat.
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
