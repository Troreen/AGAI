#pragma once

#include <cstdint>
#include <limits>

class Actor;
class GameWorld;

// --- A saved answer ---
// A cache just means keeping an answer so we do not have to fetch it again.
// Each question keeps its own computer, frame number, and debug counters.
struct PollingCache
{
    const Actor* computer = nullptr;
    // Start with a number we have not reached, so the very first question fetches an answer.
    std::uint64_t lastUpdatedFrame = (std::numeric_limits<std::uint64_t>::max)();
    std::uint64_t refreshCount = 0;
    std::uint64_t requestCount = 0;
};

// --- The place polling guards ask their questions ---
// Guards ask here rather than reading GameWorld themselves.
class AIPollingStation
{
public:
    explicit AIPollingStation(const GameWorld& aWorld);
    const Actor* GetCurrentlyHackedComputer();
    const Actor* GetLatestAttemptedComputer();

    // Debug information only: how often did we ask, and how often did we fetch?
    const PollingCache& GetCurrentCacheDebug() const;
    const PollingCache& GetLatestCacheDebug() const;
    void SetLogRefreshes(bool aEnabled);

private:
    const GameWorld& myWorld; // Non-owning; GameWorld owns this station.
    PollingCache myCurrentComputer;
    PollingCache myLatestComputer;
    bool myLogRefreshes = false;
};
