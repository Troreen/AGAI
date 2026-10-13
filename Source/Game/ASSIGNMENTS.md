# Assignment worlds

Change the number in `#define GAME_WORLD_ASSIGNMENT 5` at the top of
`source/Go.cpp`, rebuild GameMain, and run from `Bin`.

| Number | Class | Controls |
| --- | --- | --- |
| 1 | GameWorld01_Controllers | Seek, Arrive, and Wander run automatically; tune each actor in Controller Settings. |
| 2 | GameWorld02_Boids | 100 boids run automatically; tune flocking, avoidance, and debug drawing in Flocking Settings. |
| 3 | GameWorld03_WorldInterface | Left-click to move. Approach a computer to hack it; inspect the four polling/event guards. |
| 4 | GameWorld04_NavMesh | Left-click to move the player; the companion plans a path. Inspect navigation in the debug panel. |
| 5 | GameWorld05_RobotWars | Milestone 6: robot 1 detects, chases, and shoots a stationary robot 2. Space/Enter damage robot 1/2; R resets the test. Below 40 HP, robot 1 chooses Heal before combat. Defeated robots wait for reset; timed respawn and the State Machine come later. |

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

## Robot Wars milestone 4

The milestone 4 test tree followed `DecisionTree -> HealingTestDecision -> ActionNode`. A decision
checks its condition and calls `MakeDecision` on the matching child. Children can
be further decisions; an action leaf returns itself to finish the search. Each
class has its own file under `source/DecisionMaking`.

The world owns the nodes and their wander/arrive controllers. These members are
declared before ActorManager so they outlive the actor that borrows the tree.
DecisionTreeController selects an action each frame and forwards its update and
desired velocity. Actor still applies steering and movement, with wall avoidance
as a separate controller. In the milestone 4 test, H changed only the condition, so switching actions
keeps both movement controllers and their existing state.

Milestone 4 review (before the automatic condition): press H to seek the pool,
damage robot 1 with Space, observe gradual
healing, then press H again to resume wandering. Both actions should avoid the
walls. Robot 2 should keep wandering and respond to Enter. Automatic wounded
conditions, combat, and State Machine behaviour belong to later milestones.

Milestone 4 verification: Debug and Release x64 GameMain builds passed. A
standalone check using the real tree, actor, steering controllers, and engine
startup passed both branches and nested decisions, arrival within one unit of
the pool, healing capped at 100 HP, switching back to wander, and 60 seconds of
wandering inside arena bounds at 60/30/10 FPS. Interactive appearance and key
controls remain for review. The check is under ignored `Temp/DecisionTreeTests`.

## Robot Wars milestone 5

`WoundedDecision` replaces the manual `HealingTestDecision` at the root. It asks
`Actor::IsWounded()`: true leads to the healing action, false leads to Wander.
The healing check therefore has priority over normal behaviour. The later combat
milestone will put an enemy-detection decision on the normal branch.

There is no H toggle now. Press Space three times outside the pool to reduce
robot 1 from 100 to 25 HP. It should automatically choose Heal, approach the pool,
and gain 10 HP per second inside it. At 40 HP or above, it chooses Wander again.
Passive healing continues while it is still inside the pool, so it can leave with
more than 40 HP. It does not wait for full health. The label displays the action
selected for the current movement frame. Robot 2 still wanders and takes damage
with Enter; its automatic behaviour arrives with the State Machine milestone.

Milestone 5 verification: Debug and Release x64 GameMain builds passed.
`Temp/DecisionTreeTests/AutomaticHealingTests.cpp` checks disabled health,
100/40/39 HP choices, automatic seeking, gradual 10 HP/s healing, one transition
back to wandering without threshold oscillation, repeated damage, and wall
avoidance. All checks passed from three start positions (including near a wall
and inside the pool) at 60/30/10 FPS. Interactive controls and appearance await
review before committing this milestone.

## Robot Wars milestone 6

The real tree now has two questions: `WoundedDecision` first, then
`EnemyDetectedDecision` on its normal branch. Their leaves are Heal, Combat,
and Wander. Enemy detection checks for a living target within 480 units.
CombatController uses the existing arrive helper to approach a point 120 units
short of the enemy, braking across 240 units to allow for movement inertia.
Wall avoidance continues alongside all three movement actions.

The world ticks the combat cooldown every frame, including time spent healing.
Only the selected Combat action can request a shot. Bullets aim at the target's
position when fired, then travel straight at 600 units/s. They deal 15 damage,
with a 0.5-second firing interval, compared with the pool's 10 HP/s healing.
BulletSystem checks the full travelled segment against the target's radius,
consumes each hit once, and removes missed shots at the arena edge or after
two seconds. Flying shots may still hit after the shooter chooses Heal.
Bullets use the existing ninja-star sprite through SpriteDrawer in every build.

Robot 2 is a stationary target for this milestone. Both robots start within
detection range. Space damages robot 1, Enter damages robot 2, and R clears
bullets, restores HP, resets the cooldown, and returns both robots to their
starting positions. At zero HP, Actor skips AI and movement; the pool cannot
revive a defeated actor. Death is not a tree question or an action. Timed
respawn is the next milestone.

Review: watch robot 1 approach and shoot, then use R and Space three times to
check that healing interrupts combat. At 40 HP it can fight again if it detects
the target. Once the target reaches zero HP, robot 1 should wander and stop
requesting shots. R should make the whole test repeatable.

Milestone 6 verification: Debug and Release x64 GameMain built without warnings.
`Temp/DecisionTreeTests/CombatTests.cpp` passed detection boundaries, both tree
questions and all three actions, healing priority with no new shots, cooldown,
chasing without reaching the target's body, swept bullet hits, dodged shots,
lifetime expiry, consumed hits, dead-target filtering, damage exceeding pool
healing, and stopped AI/movement at zero HP. Combat movement checks passed at
60/30/10 FPS. The automatic-healing regression passed again from three start
positions at those frame rates. `CombatSceneTests.cpp` started the real graphics
engine and ran 180 Robot Wars update/render frames, including bullet sprites.
Interactive appearance and reset controls await review before committing.
