#include "WoundedDecision.h"
#include "../../Actors/Actor.h"

WoundedDecision::WoundedDecision(DecisionTreeNode& aHealingAction, DecisionTreeNode& aNormalBehaviour)
    : DecisionNode(aHealingAction, aNormalBehaviour)
{}

bool WoundedDecision::Evaluate(const Actor& aActor) const
{
    // Actor owns the HP and threshold. The node only asks whether healing is needed.
    // Once HP reaches the threshold, the false branch resumes normal behaviour.
    return aActor.IsWounded();
}
