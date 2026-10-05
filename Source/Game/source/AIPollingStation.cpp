#include "AIPollingStation.h"
#include "GameWorld.h"

#include <cstdio>

// --- Remember which world supplies our answers ---
// The station reads the world; it does not own or create it.
AIPollingStation::AIPollingStation(const GameWorld& aWorld) : myWorld(aWorld)
{
}

// --- Answer: which computer is being hacked right now? ---
// Stupid polling guards ask this every frame so they know when to stop.
const Actor* AIPollingStation::GetCurrentlyHackedComputer()
{
    // Count every question, but only fetch a new answer on the first question this frame.
    ++myCurrentComputer.requestCount;
    const std::uint64_t frame = myWorld.GetFrameCount();
    if (myCurrentComputer.lastUpdatedFrame != frame)
    {
        myCurrentComputer.computer = myWorld.GetCurrentlyHackedComputer();
        myCurrentComputer.lastUpdatedFrame = frame;
        ++myCurrentComputer.refreshCount;
        if (myLogRefreshes)
        {
            std::printf("[Polling frame %llu] refreshed current computer\n", frame);
        }
    }
    // Any more questions in this frame get the answer we already saved.
    return myCurrentComputer.computer;
}

// --- Answer: which computer did the player last try to hack? ---
// Smart polling guards use this even when the player has already left.
const Actor* AIPollingStation::GetLatestAttemptedComputer()
{
    // This answer has its own saved frame number, separate from the current computer.
    ++myLatestComputer.requestCount;
    const std::uint64_t frame = myWorld.GetFrameCount();
    if (myLatestComputer.lastUpdatedFrame != frame)
    {
        myLatestComputer.computer = myWorld.GetLatestAttemptedComputer();
        myLatestComputer.lastUpdatedFrame = frame;
        ++myLatestComputer.refreshCount;
        if (myLogRefreshes)
        {
            std::printf("[Polling frame %llu] refreshed latest computer\n", frame);
        }
    }
    return myLatestComputer.computer;
}

// --- Show how often questions were asked and answers were refreshed ---
// Reading these counters does not refresh the saved answers.
const PollingCache& AIPollingStation::GetCurrentCacheDebug() const
{
    return myCurrentComputer;
}
const PollingCache& AIPollingStation::GetLatestCacheDebug() const
{
    return myLatestComputer;
}
void AIPollingStation::SetLogRefreshes(bool aEnabled)
{
    myLogRefreshes = aEnabled;
}
