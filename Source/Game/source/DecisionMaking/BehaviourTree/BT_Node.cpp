#include "BT_Node.h"

BT_NodeResult BT_Node::Tick(Actor& aActor, float aDeltaTime, BehaviourTree& aTree)
{
    if (!myIsRunning)
    {
        Init(aActor, aDeltaTime, aTree);
    }

    return RememberResult(Update(aActor, aDeltaTime, aTree));
}

BT_NodeResult BT_Node::RememberResult(BT_NodeResult aResult)
{
    // A running node picks up where it left off on the next Tick.
    myIsRunning = aResult == BT_NodeResult::Running;
    return aResult;
}

BT_CompositeNode* BT_Node::GetParent() const
{
    return myParent;
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
