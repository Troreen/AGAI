#pragma once

class Actor;
class ActionNode;

// Both questions and actions can be children of a decision node.
class DecisionTreeNode
{
public:
    virtual ~DecisionTreeNode() = default;
    virtual ActionNode* MakeDecision(const Actor& aActor) = 0;
};
