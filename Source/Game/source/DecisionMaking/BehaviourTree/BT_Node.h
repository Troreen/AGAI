#pragma once

#include "BT_NodeResult.h"

class Actor;
class BehaviourTree;

// Each tree owns its own nodes because they remember execution between frames.
class BT_Node
{
public:
    virtual ~BT_Node() = default;

    BT_NodeResult Tick(Actor& aActor, float aDeltaTime, BehaviourTree& aTree);
    // Forget this execution. The next Tick starts with Init again.
    void Reset();

protected:
    virtual void Init(Actor& aActor, float aDeltaTime, BehaviourTree& aTree);
    virtual BT_NodeResult Update(Actor& aActor, float aDeltaTime, BehaviourTree& aTree) = 0;
    // Derived nodes can clear their own state, such as a remembered child index.
    virtual void OnReset();

private:
    bool myIsRunning = false;
};
