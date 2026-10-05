#pragma once

#include <cstdint>

// --- Give a message its event name ---
// This macro writes a small function for us. Put it inside the event struct.
// The manager uses that function to find the right listener list.
#define DECLARE_EVENT(aEventEnumType, aEventEnumEntry) \
    static constexpr aEventEnumType GetStaticType() { return aEventEnumType::aEventEnumEntry; }

// --- Make a list of event names ---
// Invalid means no valid event. Count tells us how many listener lists to make.
// Keep the names in order; their numbers are used to find those lists.
#define DECLARE_EVENT_ENUM(aEventEnumType, ...) \
    enum class aEventEnumType : std::uint8_t { Invalid, __VA_ARGS__, Count }
