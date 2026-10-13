#include "DecisionTree.h"

DecisionTree::DecisionTree(DecisionTreeNode& aRoot)
    : myRoot(aRoot)
{}

ActionNode* DecisionTree::MakeDecision(const Actor& aActor)
{
    // The root starts the chain of questions. Each chosen child continues
    // until an action leaf returns itself as the result.
    return myRoot.MakeDecision(aActor);
}
