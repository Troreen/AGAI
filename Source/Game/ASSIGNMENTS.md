# Assignment worlds

Change the number in `#define GAME_WORLD_ASSIGNMENT 4` at the top of
`source/Go.cpp`, rebuild GameMain, and run from `Bin`.

| Number | Class | Controls |
| --- | --- | --- |
| 1 | GameWorld01_Controllers | Seek, Arrive, and Wander run automatically; tune each actor in Controller Settings. |
| 2 | GameWorld02_Boids | 100 boids run automatically; tune flocking, avoidance, and debug drawing in Flocking Settings. |
| 3 | GameWorld03_WorldInterface | Left-click to move. Approach a computer to hack it; inspect the four polling/event guards. |
| 4 | GameWorld04_NavMesh | Left-click to move the player; the companion plans a path. Inspect navigation in the debug panel. |

`source/Worlds/GameWorld.h` is the small shared interface. Each numbered world owns its own
scene data and implements Init(input), Update(deltaTime), and Render(). Go keeps
one selected world on the stack, inside the existing engine lifetime. To add
another assignment, implement the interface and add a selection branch in Go.
Invalid assignment numbers produce a compile error. All implementations compile
in every build, so shared-code changes are checked against every assignment.

## Folder layout

- `source/Go.cpp` and `Go.h`: engine entry point and assignment selection.
- `source/Worlds`: common interface and one folder per assignment. Assignment
  scene helpers stay beside their world (U01 debug drawing and U03 polling/events).
- `source/Actors`: Actor and ActorManager.
- `source/Controllers`: common controller code, with Steering, Flocking,
  Navigation, and WorldInterface folders for the implementations.
- `source/Interfaces`: shared traversal bounds.
- `source/Navigation`: navmesh data, loading, and funnel code.

## Restored implementations

- Controllers: original GitHub commit `341a6d0`, adapted to the current
  CommonUtilities vectors, controller folders, and Retail debug guards. Restored
  SteeringDebugRenderer and the actor's previous steering force for its arrows.
- Boids: GitHub `Boids` branch (`b2ddc5b`), adapted to the same Init(input)
  interface and current folders. Restored multiple controllers and the separate
  steering/movement passes in Actor. FlockingSettings and the flocking/avoidance
  controllers already existed and remain shared. Settings and obstacles outlive
  the actors that borrow them.
- World Interface: GitHub `WorldInterfacing` branch (`fa1a3a9`). Restored
  AIPollingStation, AIEventManager, the four guard controllers, and
  WORLD_INTERFACING.md. These use the current CommonUtilities event manager and
  per-frame PollingCache. Player input is shared with NavMesh.
- NavMesh: copied from the working tree, including the existing local navigation
  changes, before turning GameWorld into the interface.

The worlds use the existing fish, robot, human, computer, font, and navigation
assets. No duplicate engine, input handler, or assignment-specific Actor is needed.
Debug and Release include the tuning panels and debug drawing; Retail omits them.

The Premake source glob includes the new files. The checked-in Windows Visual
Studio project also lists them, so it can build without regeneration.

## Verification

These checks were repeated after the folder cleanup.

- Each selection (1, 2, 3, 4) built and linked GameMain in Debug x64.
- Release and Retail x64 built and linked with NavMesh selected; all four worlds
  compile in each configuration. No compiler warnings or errors were reported.
- The preserved NavMesh source matches the previous implementation after renaming.
- The define and final Debug executable were returned to selection 4.
- Interactive gameplay was not checked during this architecture change.
