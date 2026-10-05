#include "WorldInterfacingControllers.h"
#include "../../Worlds/03_WorldInterface/AIPollingStation.h"
#include "../../Actors/Actor.h"
#include "../ControllerUtils.h"

// --- Movement shared by all four guards ---
namespace
{
constexpr float arrivalDistance = 4.f;
constexpr float slowingDistance = 80.f;

void StopIfIdleOrArrived(Actor& aActor, const Actor* aComputer)
{
    if (aComputer == nullptr)
    {
        aActor.Stop();
    }
    else if (aActor.GetPosition().DistanceSqr(aComputer->GetPosition()) <= arrivalDistance * arrivalDistance)
    {
        // Settle exactly at the computer so the guard does not wobble around it.
        aActor.SetPosition(aComputer->GetPosition());
        aActor.Stop();
    }
}

// Pick the direction and speed we WANT. Actor turns that into actual movement.
CommonUtilities::Vector2f GuardDesiredVelocity(const Actor& aActor, const Actor* aComputer)
{
    if (aComputer == nullptr)
    {
        return {};
    }
    return ControllerUtils::ArriveDesiredVelocity(aActor, aComputer->GetPosition(), slowingDistance);
}

// Tell the debug drawing where the guard is trying to go.
ControllerDebugInfo GuardDebugInfo(const Actor* aComputer)
{
    ControllerDebugInfo info;
    if (aComputer != nullptr)
    {
        info.hasTarget = true;
        info.targetPosition = aComputer->GetPosition();
        info.hasSlowingRadius = true;
        info.slowingRadius = slowingDistance;
    }
    return info;
}
} // namespace

// --- Smart guard: ask for the latest attempted computer ---
// The saved attempt stays available after the player leaves, so this guard keeps going.
SmartGuardPollController::SmartGuardPollController(AIPollingStation& aPollingStation)
    : myPollingStation(aPollingStation)
{
}

void SmartGuardPollController::Update(Actor& aActor, float)
{
    myTargetComputer = myPollingStation.GetLatestAttemptedComputer();
    StopIfIdleOrArrived(aActor, myTargetComputer);
}

CommonUtilities::Vector2f SmartGuardPollController::GetDesiredVelocity(const Actor& aActor) const
{
    return GuardDesiredVelocity(aActor, myTargetComputer);
}

ControllerDebugInfo SmartGuardPollController::GetDebugInfo() const
{
    return GuardDebugInfo(myTargetComputer);
}

// --- Stupid guard: ask for the computer being hacked now ---
// When the answer becomes null, the shared movement code stops this guard.
StupidGuardPollController::StupidGuardPollController(AIPollingStation& aPollingStation)
    : myPollingStation(aPollingStation)
{
}

void StupidGuardPollController::Update(Actor& aActor, float)
{
    myTargetComputer = myPollingStation.GetCurrentlyHackedComputer();
    StopIfIdleOrArrived(aActor, myTargetComputer);
}

CommonUtilities::Vector2f StupidGuardPollController::GetDesiredVelocity(const Actor& aActor) const
{
    return GuardDesiredVelocity(aActor, myTargetComputer);
}

ControllerDebugInfo StupidGuardPollController::GetDebugInfo() const
{
    return GuardDebugInfo(myTargetComputer);
}

// --- Smart guard: remember the computer from a start message ---
// It does not listen for stop messages, so leaving does not erase its destination.
SmartGuardEventController::SmartGuardEventController(AIEventManager& aEvents) : myEvents(aEvents)
{
    myEvents.RegisterEventListener(this, &SmartGuardEventController::OnPlayerStartedHacking);
}

// Stop listening before this controller is destroyed.
SmartGuardEventController::~SmartGuardEventController()
{
    myEvents.UnregisterEventListener(this);
}

// The event manager calls this when hacking starts, not on every game frame.
void SmartGuardEventController::OnPlayerStartedHacking(const PlayerStartedHackingEvent& aEvent)
{
    myTargetComputer = aEvent.computer;
    ++myReceivedEventCount;
}

void SmartGuardEventController::Update(Actor& aActor, float)
{
    StopIfIdleOrArrived(aActor, myTargetComputer);
}

CommonUtilities::Vector2f SmartGuardEventController::GetDesiredVelocity(const Actor& aActor) const
{
    return GuardDesiredVelocity(aActor, myTargetComputer);
}

ControllerDebugInfo SmartGuardEventController::GetDebugInfo() const
{
    return GuardDebugInfo(myTargetComputer);
}
unsigned int SmartGuardEventController::GetReceivedEventCount() const
{
    return myReceivedEventCount;
}

// --- Stupid guard: listen for both start and stop messages ---
// Start gives it a destination; stop takes the destination away.
StupidGuardEventController::StupidGuardEventController(AIEventManager& aEvents) : myEvents(aEvents)
{
    myEvents.RegisterEventListener(this, &StupidGuardEventController::OnPlayerStartedHacking);
    myEvents.RegisterEventListener(this, &StupidGuardEventController::OnPlayerStoppedHacking);
}

// Stop listening before this controller is destroyed.
StupidGuardEventController::~StupidGuardEventController()
{
    myEvents.UnregisterEventListener(this);
}

void StupidGuardEventController::OnPlayerStartedHacking(const PlayerStartedHackingEvent& aEvent)
{
    myTargetComputer = aEvent.computer;
    ++myReceivedEventCount;
}

// Forget the destination. The next movement update will stop the guard.
void StupidGuardEventController::OnPlayerStoppedHacking(const PlayerStoppedHackingEvent&)
{
    myTargetComputer = nullptr;
    ++myReceivedEventCount;
}

void StupidGuardEventController::Update(Actor& aActor, float)
{
    StopIfIdleOrArrived(aActor, myTargetComputer);
}

CommonUtilities::Vector2f StupidGuardEventController::GetDesiredVelocity(const Actor& aActor) const
{
    return GuardDesiredVelocity(aActor, myTargetComputer);
}

ControllerDebugInfo StupidGuardEventController::GetDebugInfo() const
{
    return GuardDebugInfo(myTargetComputer);
}
unsigned int StupidGuardEventController::GetReceivedEventCount() const
{
    return myReceivedEventCount;
}
