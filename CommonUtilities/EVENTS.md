# Events: a beginner's guide

An event is a message saying that something happened. For example, a player
can make a noise and nearby guards can react. The player sends the message;
the guards register functions that receive it.

This system delivers messages immediately. There is no update function or
event queue to process each frame.

## What changed from the teacher's code?

The teacher's headers depended on library files that this project does not have.
We kept the same basic approach and replaced those dependencies with standard C++.

| Original | Our version | Reason |
| --- | --- | --- |
| `HD_Types.h` and `u8` | `<cstdint>` and `std::uint8_t` | A standard unsigned 8-bit integer |
| `HD_GrowingArray` | `std::vector` | A listener list that can grow |
| `HD_HashMap` | `std::array` | Consecutive enum values already give each list an index |
| `HD_EventManager` | `EventManager` | Remove the teacher's library prefix |
| `std::bind` around a lambda | Just the lambda | The wrapper was unnecessary |

The reusable headers live in `CommonUtilities/include/Events/Event.hpp` and
`CommonUtilities/include/Events/EventManager.hpp`. They contain their implementations,
so there is no additional library to link. Manager types use the `CommonUtilities` namespace.

## A complete example

This example is illustrative; it does not add guards or events to the game yet.

```cpp
#include <Events/EventManager.hpp>
#include <iostream>

// Generates: Invalid = 0, NoiseHeard = 1, Count = 2.
DECLARE_EVENT_ENUM(AIEventType, NoiseHeard);

struct NoiseHeardEvent
{
    DECLARE_EVENT(AIEventType, NoiseHeard);
    float loudness = 0.f;
};

// A shorter name for a manager that handles our AI event enum.
using AIEventManager = CommonUtilities::EventManager<AIEventType>;

class Guard
{
public:
    explicit Guard(AIEventManager& aEvents)
        : myEvents(aEvents)
    {
        myEvents.RegisterEventListener(this, &Guard::OnNoiseHeard);
    }

    ~Guard()
    {
        myEvents.UnregisterEventListener(this);
    }

    // Subscriptions refer to this object's address, so keep it in one place.
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
    Guard(Guard&&) = delete;
    Guard& operator=(Guard&&) = delete;

private:
    void OnNoiseHeard(const NoiseHeardEvent& aEvent)
    {
        std::cout << "Heard loudness: " << aEvent.loudness << '\n';
    }

    AIEventManager& myEvents;
};

int main()
{
    AIEventManager events;
    Guard guard(events);

    NoiseHeardEvent noise;
    noise.loudness = 0.8f;
    events.SendEvent(noise);
} // guard is destroyed and unregisters before events is destroyed.
```

## What the macros do

`DECLARE_EVENT_ENUM` creates an enum: a set of named integer values.
`Invalid` marks a value that must not be used, and `Count` tells us how
many listener lists to allocate. Keep the entries consecutive; do not give
them custom numbers. The 8-bit enum allows at most 254 actual event entries,
because `Invalid` and `Count` also need values.

`DECLARE_EVENT` adds a small public function to an event:

```cpp
static constexpr AIEventType GetStaticType()
{
    return AIEventType::NoiseHeard;
}
```

`static` means the function can be called without an event object.
`constexpr` lets the compiler check its value before the program runs.
The manager rejects events with the wrong enum, or with `Invalid` or `Count`.

## How a message reaches a listener

1. Registering stores the listener's address and a callback in its event list.
2. Sending looks up that list using the event's enum value.
3. The manager calls each callback in registration order.
4. The callback calls the registered member function with the event.

The template lets the compiler work out the event type from the receiving
function. `const NoiseHeardEvent&` means the function reads the existing message
without copying it or changing it.

Internally, `std::function` stores a callable function. A lambda captures the
listener's address and its member function. The teacher's `void*` approach
remains: it stores different event payloads behind a generic pointer, then
casts back to the expected event type inside the callback.

**Use one concrete event struct per enum entry.** Two different structs with
the same entry would make that cast unsafe. The compiler's enum checks do not
detect that mistake.

## Keeping delivery simple

The manager stores subscription records by value in a vector and loops over
them directly when sending an event. It does not need shared pointers,
snapshots, active flags, or a deferred registration queue.

Register listeners when controllers are created and unregister them when
controllers are destroyed. Do not register, unregister, or destroy listeners
inside an event callback: that would change the vector while it is being read.
Receiving functions should handle the message without changing subscriptions.

## Object lifetime and usage rules

- Unregister before a listener is destroyed.
- Keep the manager alive longer than its listeners and until its sends return.
- Do not move or copy a registered listener: the stored address would be wrong.
- Use this manager on one thread; it does not synchronize concurrent access.
- Do not store a reference to an event after the callback returns. Copy needed data.
- Avoid endlessly sending the same event from its own callback: nested sends recurse.

The manager itself cannot be copied or moved, preventing accidental duplication
of subscriptions. A game world can own it and pass references to interested objects.
The former U03 scene used this same manager for hacking transitions; U04 can
reuse it later for gameplay events without bringing that scene back.
