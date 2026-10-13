#include "BT_Node.h"

BT_NodeResult BT_Node::Tick(Actor& aActor, float aDeltaTime, BehaviourTree& aTree)
{
    if (!myIsRunning)
    {
        Init(aActor, aDeltaTime, aTree);
    }

    const BT_NodeResult result = Update(aActor, aDeltaTime, aTree);
    // A running node picks up where it left off on the next Tick.
    myIsRunning = result == BT_NodeResult::Running;
    return result;
}

void BT_Node::Reset()
{
    myIsRunning = false;
    OnReset();
}

void BT_Node::Init(Actor&, float, BehaviourTree&)
{}

void BT_Node::OnReset()
{}
