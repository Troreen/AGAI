#pragma once

#include "BT_LeafNodeAction.h"

class Controller;

// Borrow an existing ongoing behaviour such as Arrive, Combat or Wander.
// The controller must outlive this leaf; the tree decides when to tick it.
class BT_ControllerAction final : public BT_LeafNodeAction
{
public:
    explicit BT_ControllerAction(Controller& aController);
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;

protected:
    BT_NodeResult Execute(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) override;

private:
    Controller& myController;
};
