#include "DecisionTree.h"

DecisionTree::DecisionTree(DecisionTreeNode& aRoot)
    : myRoot(aRoot)
{}

ActionNode* DecisionTree::MakeDecision(const Actor& aActor)
{
    return myRoot.MakeDecision(aActor);
}
