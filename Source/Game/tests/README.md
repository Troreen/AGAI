Run the obstacle-avoidance tests from an x64 Visual Studio Developer Command Prompt at the repository root:

Run the standalone event-manager tests from the same prompt:

```bat
if not exist Temp\EventManagerTests mkdir Temp\EventManagerTests
cl /nologo /std:c++20 /EHsc /W4 /WX Source/Game/tests/EventManagerTests.cpp /Fe:Temp/EventManagerTests/EventManagerTests.exe /Fo:Temp/EventManagerTests/
Temp\EventManagerTests\EventManagerTests.exe
```

These tests do not need the engine or CommonUtilities libraries. The beginner's
guide is in `Source/Game/EVENTS.md`.

Obstacle-avoidance tests:

```bat
if not exist Temp\ObstacleAvoidanceTests mkdir Temp\ObstacleAvoidanceTests
cl /nologo /std:c++20 /EHsc /W4 /WX /I CommonUtilities/include Source/Game/source/ObstacleAvoidance.cpp Source/Game/source/Interfaces/TraversalBounds.cpp Source/Game/tests/ObstacleAvoidanceTests.cpp /Fe:Temp/ObstacleAvoidanceTests/ObstacleAvoidanceTests.exe /Fo:Temp/ObstacleAvoidanceTests/
Temp\ObstacleAvoidanceTests\ObstacleAvoidanceTests.exe
```

These tests exercise finite rays, nearest hits, collision radius, tangents, escape direction selection, rotated headings, overlap recovery, and fully blocked paths without starting the renderer. They also check that containment defaults to off.

The local `Local/SteeringSpeedTests.vcxproj` builds `SteeringSpeedTests.cpp` against the game libraries. Build it with `msbuild Local/SteeringSpeedTests.vcxproj /p:Configuration=Debug /p:Platform=x64`, then run `Bin/SteeringSpeedTests_Debug.exe`. It checks that inactive flocking does not brake, wandering retains its speed without neighbours, and wall avoidance alone does not increase speed over 120 frames.
