#pragma once

#include <cstdint>

namespace CommonUtilities
{
// One saved answer per question. The caller supplies the frame and how to fetch
// the answer; this utility knows nothing about actors, computers, or GameWorld.
template <typename ValueType>
class PollingCache
{
public:
    template <typename FetchFunction>
    const ValueType& Get(std::uint64_t aFrame, const FetchFunction& aFetch)
    {
        ++myRequestCount;
        if (!myHasValue || myLastUpdatedFrame != aFrame)
        {
            myValue = aFetch();
            myLastUpdatedFrame = aFrame;
            myHasValue = true;
            ++myRefreshCount;
        }
        return myValue;
    }

    // Use when a world is reset or its frame counter starts over.
    void Invalidate() { myHasValue = false; }
    std::uint64_t GetLastUpdatedFrame() const { return myLastUpdatedFrame; }
    std::uint64_t GetRequestCount() const { return myRequestCount; }
    std::uint64_t GetRefreshCount() const { return myRefreshCount; }

private:
    ValueType myValue{};
    bool myHasValue = false;
    std::uint64_t myLastUpdatedFrame = 0;
    std::uint64_t myRequestCount = 0;
    std::uint64_t myRefreshCount = 0;
};
}
