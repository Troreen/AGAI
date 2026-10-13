#pragma once

#include "DecisionTreeNode.h"

// Start each decision at the root. All linked nodes must outlive the tree.
class DecisionTree
{
public:
    explicit DecisionTree(DecisionTreeNode& aRoot);
    ActionNode* MakeDecision(const Actor& aActor);

private:
    DecisionTreeNode& myRoot;
};
