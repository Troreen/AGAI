#pragma once

#include "../Controller.h"

class BehaviourTree;

// Actor keeps the usual steering pass; the tree supplies its active movement.
class BehaviourTreeController final : public Controller
{
public:
    explicit BehaviourTreeController(BehaviourTree& aTree);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;

private:
    BehaviourTree& myTree;
};
