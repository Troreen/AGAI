#pragma once

#include "BT_NodeResult.h"

class Actor;
class BehaviourTree;
class BT_CompositeNode;

// Each tree owns its own nodes because they remember execution between frames.
class BT_Node
{
public:
    BT_Node() = default;
    virtual ~BT_Node() = default;
    // Linked nodes stay at the same address for the lifetime of their tree.
    BT_Node(const BT_Node&) = delete;
    BT_Node& operator=(const BT_Node&) = delete;
    BT_Node(BT_Node&&) = delete;
    BT_Node& operator=(BT_Node&&) = delete;

    BT_NodeResult Tick(Actor& aActor, float aDeltaTime, BehaviourTree& aTree);
    // Forget this execution. The next Tick starts with Init again.
    void Reset();
    BT_CompositeNode* GetParent() const;

protected:
    virtual void Init(Actor& aActor, float aDeltaTime, BehaviourTree& aTree);
    virtual BT_NodeResult Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) = 0;
    // Derived nodes can clear their own state, such as a remembered child index.
    virtual void OnReset();
    // Continuation finishes a composite tick without calling Init again.
    BT_NodeResult RememberResult(BT_NodeResult aResult);

private:
    friend class BT_CompositeNode;
    BT_CompositeNode* myParent = nullptr; // Borrowed; the tree owns both nodes.
    bool myIsRunning = false;
};
