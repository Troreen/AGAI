#pragma once

#include "Event.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>

namespace CommonUtilities
{
// --- The kind of function a listener gives us ---
// It belongs to the listener and reads one message of the matching event type.
template <typename ListenerType, typename EventChildType>
using EventReceivedCallback = void (ListenerType::*)(const EventChildType&);

// --- One entry on the listener list ---
// Remember who wants messages and which function to call.
// The manager does not own that object. Remove its entry before destroying it.
struct EventListener
{
    void* myListener = nullptr;
    std::function<void(const void*)> myCallback;
};

// --- Deliver messages to the objects that signed up for them ---
// Each event name has its own listener list.
template <typename EventEnumType>
class EventManager
{
    static_assert(std::is_enum_v<EventEnumType>, "EventManager needs an event enum.");

public:
    EventManager() = default;
    // Keep one manager in one place so we do not accidentally copy its listener lists.
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;
    EventManager(EventManager&&) = delete;
    EventManager& operator=(EventManager&&) = delete;

    // --- Sign up to receive a particular kind of message ---
    template <typename ListenerType, typename EventChildType>
    void RegisterEventListener(ListenerType* aListener, EventReceivedCallback<ListenerType, EventChildType> aCallback)
    {
        assert(aListener != nullptr && aCallback != nullptr);
        EventListener subscription;
        subscription.myListener = aListener;
        // Save a small function that calls the listener's own receiving function.
        // The list uses a general pointer; this function turns it back into the
        // specific message type that this listener expects.
        subscription.myCallback = [aListener, aCallback](const void* aEvent)
        { (aListener->*aCallback)(*static_cast<const EventChildType*>(aEvent)); };

        myEventListeners[GetEventIndex<EventChildType>()].push_back(subscription);
    }

    // --- Stop sending messages to this object ---
    // Remove all of its entries, including entries for different event names.
    void UnregisterEventListener(void* aListener)
    {
        for (std::vector<EventListener>& listeners : myEventListeners)
        {
            std::erase_if(listeners, [aListener](const EventListener& subscription)
                          { return subscription.myListener == aListener; });
        }
    }

    // --- Send a message now ---
    // Call each listener in the order it signed up. There is no message queue.
    // Receiving functions must not change the listener lists while this loop is running.
    template <typename EventChildType>
    void SendEvent(const EventChildType& aEvent)
    {
        const std::vector<EventListener>& listeners = myEventListeners[GetEventIndex<EventChildType>()];
        for (const EventListener& subscription : listeners)
        {
            subscription.myCallback(&aEvent);
        }
    }

private:
    // --- Find the listener list for this message ---
    // The compiler checks that the event name belongs to this manager and is valid.
    template <typename EventChildType>
    static constexpr std::size_t GetEventIndex()
    {
        static_assert(std::is_same_v<decltype(EventChildType::GetStaticType()), EventEnumType>,
                      "The event must use this manager's enum.");
        constexpr std::size_t index = static_cast<std::size_t>(EventChildType::GetStaticType());
        static_assert(index > static_cast<std::size_t>(EventEnumType::Invalid) &&
                          index < static_cast<std::size_t>(EventEnumType::Count),
                      "The event must use an entry between Invalid and Count.");
        return index;
    }

    // One growable listener list per enum value. Invalid's list stays unused.
    std::array<std::vector<EventListener>, static_cast<std::size_t>(EventEnumType::Count)> myEventListeners;
};

} // namespace CommonUtilities
