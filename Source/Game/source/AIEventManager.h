#pragma once

#include "EventManager.h"

class Actor;

// --- The two messages that can be sent ---
// An event is a message saying something changed, not a message sent every frame.
DECLARE_EVENT_ENUM(AIEventType, PlayerStartedHacking, PlayerStoppedHacking);

// --- Hacking has just started ---
// The message tells guards which computer the player approached.
struct PlayerStartedHackingEvent
{
    DECLARE_EVENT(AIEventType, PlayerStartedHacking);
    const Actor* computer = nullptr; // Points to GameWorld's computer; does not create a copy.
};

// --- Hacking has just stopped ---
// This identifies the computer the player left.
struct PlayerStoppedHackingEvent
{
    DECLARE_EVENT(AIEventType, PlayerStoppedHacking);
    const Actor* computer = nullptr;
};

// --- A short name for the manager that delivers these messages ---
using AIEventManager = EventManager<AIEventType>;
