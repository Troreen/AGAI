#pragma once

#include "DecisionTreeNode.h"
#include <Math/Vector2.hpp>

class Controller;

// A leaf represents a movement action using an existing steering controller.
class ActionNode : public DecisionTreeNode
{
public:
    explicit ActionNode(Controller& aMovementController);
    ActionNode* MakeDecision(const Actor& aActor) override;
    virtual void Update(Actor& aActor, float aDeltaTime);
    virtual CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const;

private:
    Controller& myMovementController;
};
