#include "DecisionNode.h"

DecisionNode::DecisionNode(DecisionTreeNode& aTrueChild, DecisionTreeNode& aFalseChild)
    : myTrueChild(aTrueChild), myFalseChild(aFalseChild)
{}

ActionNode* DecisionNode::MakeDecision(const Actor& aActor)
{
    // Answer this question, then continue down the selected branch.
    // Another decision asks its own question; an action ends the traversal.
    if (Evaluate(aActor))
    {
        return myTrueChild.MakeDecision(aActor);
    }

    return myFalseChild.MakeDecision(aActor);
}
