#pragma once

#include "DecisionTreeNode.h"

// A question chooses between two branches. The nodes are owned by the caller.
class DecisionNode : public DecisionTreeNode
{
public:
    DecisionNode(DecisionTreeNode& aTrueChild, DecisionTreeNode& aFalseChild);
    ActionNode* MakeDecision(const Actor& aActor) override;

protected:
    virtual bool Evaluate(const Actor& aActor) const = 0;

private:
    DecisionTreeNode& myTrueChild;
    DecisionTreeNode& myFalseChild;
};
