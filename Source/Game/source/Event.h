#pragma once

#include <cstdint>

// Put DECLARE_EVENT in the public section of each event class or struct.
#define DECLARE_EVENT(aEventEnumType, aEventEnumEntry) \
    static constexpr aEventEnumType GetStaticType() { return aEventEnumType::aEventEnumEntry; }

// Keep entries consecutive: their values are used as listener-list indices.
#define DECLARE_EVENT_ENUM(aEventEnumType, ...) \
    enum class aEventEnumType : std::uint8_t { Invalid, __VA_ARGS__, Count }
