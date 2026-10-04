#pragma once

#include "Event.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

template<typename ListenerType, typename EventChildType>
using EventReceivedCallback = void (ListenerType::*)(const EventChildType&);

// A subscription stores the listener's address and the function to call.
// It does not own the listener: unregister before destroying that object.
struct EventListener
{
    void* myListener = nullptr;
    std::function<void(const void*)> myCallback;
    bool myIsActive = true;
};

template<typename EventEnumType>
class EventManager
{
    static_assert(std::is_enum_v<EventEnumType>, "EventManager needs an event enum.");

public:
    EventManager() = default;
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;
    EventManager(EventManager&&) = delete;
    EventManager& operator=(EventManager&&) = delete;

    template<typename ListenerType, typename EventChildType>
    void RegisterEventListener(
        ListenerType* aListener,
        EventReceivedCallback<ListenerType, EventChildType> aCallback)
    {
        assert(aListener != nullptr && aCallback != nullptr);
        if (aListener == nullptr || aCallback == nullptr)
        {
            return;
        }

        auto subscription = std::make_shared<EventListener>();
        subscription->myListener = aListener;
        subscription->myCallback = [aListener, aCallback](const void* aEvent)
        {
            (aListener->*aCallback)(*static_cast<const EventChildType*>(aEvent));
        };

        myEventListeners[GetEventIndex<EventChildType>()].push_back(subscription);
    }

    // Remove every subscription belonging to this listener, across all event types.
    void UnregisterEventListener(void* aListener)
    {
        for (auto& listeners : myEventListeners)
        {
            std::erase_if(listeners, [aListener](const auto& subscription)
            {
                if (subscription->myListener != aListener)
                {
                    return false;
                }

                // A SendEvent snapshot may still hold this subscription.
                subscription->myIsActive = false;
                return true;
            });
        }
    }

    // Delivery happens immediately, in registration order, on the calling thread.
    template<typename EventChildType>
    void SendEvent(const EventChildType& aEvent)
    {
        // Copy the list of subscription handles so callbacks can safely change
        // registrations. Newly registered listeners join the next SendEvent.
        const auto listeners = myEventListeners[GetEventIndex<EventChildType>()];
        for (const auto& subscription : listeners)
        {
            if (subscription->myIsActive)
            {
                subscription->myCallback(&aEvent);
            }
        }
    }

private:
    template<typename EventChildType>
    static constexpr std::size_t GetEventIndex()
    {
        static_assert(
            std::is_same_v<decltype(EventChildType::GetStaticType()), EventEnumType>,
            "The event must use this manager's enum.");
        constexpr auto index = static_cast<std::size_t>(EventChildType::GetStaticType());
        static_assert(
            index > static_cast<std::size_t>(EventEnumType::Invalid) &&
            index < static_cast<std::size_t>(EventEnumType::Count),
            "The event must use an entry between Invalid and Count.");
        return index;
    }

    // One growable listener list per enum value. Invalid's list stays unused.
    std::array<std::vector<std::shared_ptr<EventListener>>,
        static_cast<std::size_t>(EventEnumType::Count)> myEventListeners;
};
