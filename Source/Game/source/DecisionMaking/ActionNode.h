#pragma once

#include "DecisionTreeNode.h"
#include <Math/Vector2.hpp>

class Controller;

// A leaf is the tree's answer: Wander, Heal, or Combat. It borrows an existing
// controller for movement and any extra action, such as shooting.
class ActionNode : public DecisionTreeNode
{
public:
    explicit ActionNode(Controller& aActionController);
    ActionNode* MakeDecision(const Actor& aActor) override;
    virtual void Update(Actor& aActor, float aDeltaTime);
    virtual CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const;

private:
    Controller& myActionController;
};
