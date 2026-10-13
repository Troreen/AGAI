#pragma once

#include "BT_NodeResult.h"
#include <cstddef>
#include <vector>

class Actor;
class BT_Node;
class BT_LeafNodeAction;
class BT_LeafNodeCondition;

// Borrow the root and remember the action reached on the last traversal.
// Nodes and this runtime belong to one actor and stay alive together.
class BehaviourTree
{
public:
    explicit BehaviourTree(BT_Node& aRoot);
    // One runtime tracks the execution of its own borrowed nodes.
    BehaviourTree(const BehaviourTree&) = delete;
    BehaviourTree& operator=(const BehaviourTree&) = delete;
    BehaviourTree(BehaviourTree&&) = delete;
    BehaviourTree& operator=(BehaviourTree&&) = delete;
    BT_NodeResult Run(Actor& aActor, float aDeltaTime);
    void Reset();
    void RememberCondition(const BT_LeafNodeCondition& aCondition, bool aResult);
    void RememberAction(BT_LeafNodeAction& aAction);
    const BT_LeafNodeAction* GetRunningAction() const;
    std::size_t GetRootTraversalCount() const;

private:
    struct CheckedCondition
    {
        const BT_LeafNodeCondition* condition;
        bool result;
    };
    bool HaveConditionsChanged(const Actor& aActor) const;
    BT_Node& myRoot;
    BT_LeafNodeAction* myRunningAction = nullptr;
    std::vector<CheckedCondition> myConditions;
    std::size_t myRootTraversalCount = 0;
};
