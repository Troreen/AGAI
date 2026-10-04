#include "../source/EventManager.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

DECLARE_EVENT_ENUM(TestEventType, Ping, Other);

struct PingEvent
{
    DECLARE_EVENT(TestEventType, Ping);
    int value = 0;
};

struct OtherEvent
{
    DECLARE_EVENT(TestEventType, Other);
};

using TestEventManager = EventManager<TestEventType>;

struct Receiver
{
    int total = 0;
    int otherCount = 0;
    std::function<void()> onPing;

    void OnPing(const PingEvent& aEvent)
    {
        total += aEvent.value;
        if (onPing) onPing();
    }

    void OnOther(const OtherEvent&)
    {
        ++otherCount;
    }
};

int main()
{
    static_assert(!std::is_copy_constructible_v<TestEventManager>);
    static_assert(!std::is_move_constructible_v<TestEventManager>);

    {
        TestEventManager manager;
        Receiver receiver;
        manager.SendEvent(PingEvent{ 5 }); // No listeners is fine.
        manager.RegisterEventListener(&receiver, &Receiver::OnPing);
        manager.RegisterEventListener(&receiver, &Receiver::OnOther);
        manager.SendEvent(PingEvent{ 5 });
        assert(receiver.total == 5 && receiver.otherCount == 0);
        manager.SendEvent(OtherEvent{});
        assert(receiver.otherCount == 1);
        manager.RegisterEventListener(&receiver, &Receiver::OnPing);
        manager.UnregisterEventListener(&receiver); // Removes both Ping registrations.
        manager.UnregisterEventListener(&receiver); // Repeating removal is fine.
        manager.SendEvent(PingEvent{ 5 });
        manager.SendEvent(OtherEvent{});
        assert(receiver.total == 5 && receiver.otherCount == 1);
    }

    {
        TestEventManager manager;
        Receiver first;
        Receiver added;
        auto removed = std::make_unique<Receiver>();
        first.onPing = [&]
        {
            manager.UnregisterEventListener(removed.get());
            removed.reset(); // A removed listener can be destroyed before its turn.
            manager.UnregisterEventListener(&first);
            manager.RegisterEventListener(&added, &Receiver::OnPing);
        };
        manager.RegisterEventListener(&first, &Receiver::OnPing);
        manager.RegisterEventListener(removed.get(), &Receiver::OnPing);
        manager.SendEvent(PingEvent{ 1 });
        assert(first.total == 1 && added.total == 0);
        manager.SendEvent(PingEvent{ 2 });
        assert(first.total == 1 && added.total == 2);
    }

    {
        TestEventManager manager;
        Receiver first;
        Receiver added;
        first.onPing = [&]
        {
            manager.UnregisterEventListener(&first);
            manager.RegisterEventListener(&added, &Receiver::OnPing);
            manager.SendEvent(PingEvent{ 10 }); // Nested sends see current registrations.
        };
        manager.RegisterEventListener(&first, &Receiver::OnPing);
        manager.SendEvent(PingEvent{ 1 });
        assert(first.total == 1 && added.total == 10);
    }

    {
        TestEventManager manager;
        Receiver receiver;
        receiver.onPing = [] { throw std::runtime_error("test"); };
        manager.RegisterEventListener(&receiver, &Receiver::OnPing);
        bool caught = false;
        try { manager.SendEvent(PingEvent{ 1 }); }
        catch (const std::runtime_error&) { caught = true; }
        assert(caught);
        receiver.onPing = {};
        manager.SendEvent(PingEvent{ 2 });
        assert(receiver.total == 3);
    }

    std::cout << "Event manager tests passed.\n";
}
