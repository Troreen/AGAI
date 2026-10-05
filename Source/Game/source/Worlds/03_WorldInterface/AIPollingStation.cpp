#include "AIPollingStation.h"
#include "GameWorld03_WorldInterface.h"

#include <cstdio>

// --- Remember which world supplies our answers ---
// The station reads the world; it does not own or create it.
AIPollingStation::AIPollingStation(const GameWorld03_WorldInterface& aWorld) : myWorld(aWorld)
{
}

// --- Answer: which computer is being hacked right now? ---
// Stupid polling guards ask this every frame so they know when to stop.
const Actor* AIPollingStation::GetCurrentlyHackedComputer()
{
    const std::uint64_t frame = myWorld.GetFrameCount();
    return myCurrentComputer.Get(frame, [this, frame]()
    {
        if (myLogRefreshes)
            std::printf("[Polling frame %llu] refreshed current computer\n", frame);
        return myWorld.GetCurrentlyHackedComputer();
    });
}

// --- Answer: which computer did the player last try to hack? ---
// Smart polling guards use this even when the player has already left.
const Actor* AIPollingStation::GetLatestAttemptedComputer()
{
    const std::uint64_t frame = myWorld.GetFrameCount();
    return myLatestComputer.Get(frame, [this, frame]()
    {
        if (myLogRefreshes)
            std::printf("[Polling frame %llu] refreshed latest computer\n", frame);
        return myWorld.GetLatestAttemptedComputer();
    });
}

// --- Show how often questions were asked and answers were refreshed ---
// Reading these counters does not refresh the saved answers.
const AIPollingCache& AIPollingStation::GetCurrentCacheDebug() const
{
    return myCurrentComputer;
}
const AIPollingCache& AIPollingStation::GetLatestCacheDebug() const
{
    return myLatestComputer;
}
void AIPollingStation::SetLogRefreshes(bool aEnabled)
{
    myLogRefreshes = aEnabled;
}
