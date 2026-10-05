#pragma once

#include "../../Worlds/03_WorldInterface/AIEventManager.h"
#include "../Controller.h"

class AIPollingStation;

// --- The four guard brains ---
// Smart guards remember the latest attempt; stupid guards only follow current hacking.
// Polling means asking for information. Events mean receiving a message when it changes.
// Computer pointers refer to the existing computers; the guards do not own them.
class SmartGuardPollController final : public Controller
{
public:
    explicit SmartGuardPollController(AIPollingStation& aPollingStation);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    ControllerDebugInfo GetDebugInfo() const override;

private:
    AIPollingStation& myPollingStation;
    const Actor* myTargetComputer = nullptr;
};

// --- Stupid guard that asks the polling station ---
class StupidGuardPollController final : public Controller
{
public:
    explicit StupidGuardPollController(AIPollingStation& aPollingStation);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    ControllerDebugInfo GetDebugInfo() const override;

private:
    AIPollingStation& myPollingStation;
    const Actor* myTargetComputer = nullptr;
};

// --- Smart guard that listens for hacking-started messages ---
class SmartGuardEventController final : public Controller
{
public:
    explicit SmartGuardEventController(AIEventManager& aEvents);
    ~SmartGuardEventController() override;
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    ControllerDebugInfo GetDebugInfo() const override;
    unsigned int GetReceivedEventCount() const;

private:
    void OnPlayerStartedHacking(const PlayerStartedHackingEvent& aEvent);
    AIEventManager& myEvents;
    const Actor* myTargetComputer = nullptr;
    unsigned int myReceivedEventCount = 0;
};

// --- Stupid guard that listens for hacking-started and hacking-stopped messages ---
class StupidGuardEventController final : public Controller
{
public:
    explicit StupidGuardEventController(AIEventManager& aEvents);
    ~StupidGuardEventController() override;
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    ControllerDebugInfo GetDebugInfo() const override;
    unsigned int GetReceivedEventCount() const;

private:
    void OnPlayerStartedHacking(const PlayerStartedHackingEvent& aEvent);
    void OnPlayerStoppedHacking(const PlayerStoppedHackingEvent& aEvent);
    AIEventManager& myEvents;
    const Actor* myTargetComputer = nullptr;
    unsigned int myReceivedEventCount = 0;
};
