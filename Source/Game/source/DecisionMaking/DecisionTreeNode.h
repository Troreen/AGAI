#pragma once

class Actor;
class ActionNode;

// Both questions and actions can be children of a decision node.
class DecisionTreeNode
{
public:
    virtual ~DecisionTreeNode() = default;
    // Questions continue to a child; leaves return the action to run this frame.
    virtual ActionNode* MakeDecision(const Actor& aActor) = 0;
};
