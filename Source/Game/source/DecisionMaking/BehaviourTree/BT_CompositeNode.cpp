#include "BT_CompositeNode.h"

void BT_CompositeNode::AddChild(BT_Node& aChild)
{
    myChildren.push_back(&aChild);
    aChild.myParent = this;
}

BT_NodeResult BT_CompositeNode::Continue(Actor& aActor, float aDeltaTime,
    BehaviourTree& aTree, BT_NodeResult aChildResult)
{
    return RememberResult(ContinueFromChild(aActor, aDeltaTime, aTree, aChildResult));
}

void BT_CompositeNode::Init(Actor&, float, BehaviourTree&)
{
    myCurrentChildIndex = 0;
}

void BT_CompositeNode::OnReset()
{
    myCurrentChildIndex = 0;
    for (BT_Node* child : myChildren)
    {
        child->Reset();
    }
}
