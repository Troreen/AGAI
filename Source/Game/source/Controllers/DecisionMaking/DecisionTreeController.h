#pragma once

#include "../Controller.h"

class DecisionTree;
class ActionNode;

// Connect the tree's chosen action to the actor's normal steering pass.
class DecisionTreeController final : public Controller
{
public:
    explicit DecisionTreeController(DecisionTree& aDecisionTree);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;

private:
    DecisionTree& myDecisionTree;
    ActionNode* mySelectedAction = nullptr;
};
