#include "BehaviourTree.h"
#include "BT_CompositeNode.h"
#include "BT_LeafNodeAction.h"
#include "BT_LeafNodeCondition.h"

BehaviourTree::BehaviourTree(BT_Node& aRoot) : myRoot(aRoot)
{}

BT_NodeResult BehaviourTree::Run(Actor& aActor, float aDeltaTime)
{
    if (HaveConditionsChanged(aActor))
    {
        // Reset before updating any action, so a new priority interrupts now.
        Reset();
    }

    BT_NodeResult result;
    if (myRunningAction)
    {
        BT_Node* completedNode = myRunningAction;
        result = completedNode->Tick(aActor, aDeltaTime, *this);
        // A finished action hands its result to its parent. That parent can
        // visit a sibling or finish too, without replaying completed children.
        while (result != BT_NodeResult::Running && completedNode->GetParent())
        {
            BT_CompositeNode* parent = completedNode->GetParent();
            result = parent->Continue(aActor, aDeltaTime, *this, result);
            completedNode = parent;
        }
    }
    else
    {
        myConditions.clear();
        ++myRootTraversalCount;
        result = myRoot.Tick(aActor, aDeltaTime, *this);
    }

    if (result != BT_NodeResult::Running)
    {
        myRunningAction = nullptr;
        myConditions.clear();
    }
    return result;
}

bool BehaviourTree::HaveConditionsChanged(const Actor& aActor) const
{
    for (const CheckedCondition& checked : myConditions)
    {
        if (checked.condition->Evaluate(aActor) != checked.result)
            return true;
    }
    return false;
}

void BehaviourTree::Reset()
{
    myRoot.Reset();
    myRunningAction = nullptr;
    myConditions.clear();
    // Keep the counter across resets so debug information remains cumulative.
}

void BehaviourTree::RememberCondition(const BT_LeafNodeCondition& aCondition, bool aResult)
{
    myConditions.push_back({&aCondition, aResult});
}

void BehaviourTree::RememberAction(BT_LeafNodeAction& aAction)
{
    myRunningAction = &aAction;
}

const BT_LeafNodeAction* BehaviourTree::GetRunningAction() const
{
    return myRunningAction;
}

std::size_t BehaviourTree::GetRootTraversalCount() const
{
    return myRootTraversalCount;
}
