#pragma once

#include <cstdint>
#include <Events/PollingCache.hpp>

class Actor;
class GameWorld03_WorldInterface;

using AIPollingCache = CommonUtilities::PollingCache<const Actor*>;

// --- The place polling guards ask their questions ---
// Guards ask here rather than reading GameWorld03_WorldInterface themselves.
class AIPollingStation
{
public:
    explicit AIPollingStation(const GameWorld03_WorldInterface& aWorld);
    const Actor* GetCurrentlyHackedComputer();
    const Actor* GetLatestAttemptedComputer();

    // Debug information only: how often did we ask, and how often did we fetch?
    const AIPollingCache& GetCurrentCacheDebug() const;
    const AIPollingCache& GetLatestCacheDebug() const;
    void SetLogRefreshes(bool aEnabled);

private:
    const GameWorld03_WorldInterface& myWorld; // Non-owning; GameWorld03_WorldInterface owns this station.
    AIPollingCache myCurrentComputer;
    AIPollingCache myLatestComputer;
    bool myLogRefreshes = false;
};
