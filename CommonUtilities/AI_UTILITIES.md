# Reusable events and polling

The U03 guard/computer scene has been removed. Its general event delivery and
once-per-frame polling cache now live in CommonUtilities and have no GameWorld
or Actor dependency. U04 does not need to send hacking events or poll computers.

`include/Event.hpp` keeps the course event declaration macros.
`include/EventManager.hpp` keeps registration, synchronous delivery in registration
order, and explicit unregistering. Its classes are in `CommonUtilities`.
See [EVENTS.md](EVENTS.md) for the full explanation and a complete example.

`include/PollingCache.hpp` preserves the polling station's cache behaviour:
each question has its own answer and frame number. Only its first request in
a frame fetches fresh data; later requests reuse that answer. Request and
refresh counters remain available for debugging.

```cpp
#include <PollingCache.hpp>

CommonUtilities::PollingCache<int> actorCount;
// Ask with the current frame and a function that supplies the answer.
const int count = actorCount.Get(frame, [&world]() { return world.GetActorCount(); });
```

Make one cache member for each question in a game's polling station. Update the
world before asking. Call `Invalidate()` when resetting the world/frame counter.
Pointer answers are non-owning: the caller must keep the referenced objects alive.
The returned reference stays valid until the cache is destroyed, but its value
changes on refresh. Copy the answer when a caller needs its own snapshot.

These utilities are header-only and use the existing CommonUtilities include path.
Game's Premake script and Visual Studio project list them explicitly.
