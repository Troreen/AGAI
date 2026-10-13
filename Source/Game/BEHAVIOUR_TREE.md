# U06 Behaviour Tree

## Milestone progress

- 1: node results and execution lifecycle committed as `814e87a`.
- 2: ordered composites and completion continuation committed as `3fe4818`.
- 3: condition and action leaves committed as `2c02b2a`.
- 4: persistent runtime and steering integration committed as `50640b7`.
- 5: three-robot scene and actual tree committed as `1d40261`.
- 6: final verification and reading guide complete; review and fixes verified.

## Persistent execution

`BehaviourTree` borrows one root. The actor's tree owns all its nodes for the
whole runtime lifetime. During traversal, condition leaves record their answers,
including failed questions in earlier Selector branches. Action leaves remember
the action that was reached, then call `Execute` to perform its behaviour.

On the next frame, `Run` compares the recorded questions with their old answers.
If they agree, it ticks the running action directly. Neither the root nor its
composites are ticked again. If a question changes, the runtime resets the old
execution and traverses from the root before an action updates that frame.

A completed action passes Success or Failure to its parent through `Continue`.
The parent resumes at its remembered child index and may visit another sibling.
Results keep travelling upward until another action runs or the root completes.
The next frame after root completion starts a new execution.

`BehaviourTreeController` calls `Run` in the existing controller update pass.
It then supplies the running action's desired velocity to Actor's steering.
A completed tree supplies zero velocity. Battle reset clears execution while
preserving the cumulative root-traversal count for debugging.

## Milestone 4 reading order

Read each header before its implementation:

1. `DecisionMaking/BehaviourTree/BT_NodeResult.h`, then `BT_Node`: lifecycle.
2. `BT_CompositeNode`, `BT_Sequence`, `BT_Selector`: ordered children and continuation.
3. `BT_LeafNodeCondition`, `BT_LeafNodeAction`, `BT_ControllerAction`: recording
   questions, remembering actions, and delegating ongoing movement.
4. `BehaviourTree`: `Run`, condition comparison, completion propagation, `Reset`.
5. `Controllers/DecisionMaking/BehaviourTreeController`: `Update` and movement query.

Each tree instance belongs to one actor. Shared controllers and nodes cannot be
used for simultaneous executions because they remember their own running state.

## Three-robot scene

Select `GAME_WORLD_ASSIGNMENT 6` in `source/Go.cpp`. The thin
`GameWorld06_BehaviourTree` subclass enables a third robot in the shared arena.
Selector 5 still creates the original two-robot battle with fixed enemies.

`RobotWarsBehaviourTree` owns Sir Killalot's controllers, conditions, composites,
action adapters, and runtime. Its Selector visits these branches in order:

```text
Selector
  Sequence: Wounded condition -> Heal action
  Sequence: Enemy detected condition -> Combat action
  Wander action
```

The concrete conditions are leaves. Heal borrows Arrive, Combat borrows the
existing chase/shoot controller, and Wander borrows its own seeded controller.
Each selected action stays Running. HP below 40 selects Heal; at 40 or above,
Combat or Wander resumes. The shared pool restores 10 HP/s up to 100 HP.

Before movement, each U06 combat controller selects its nearest living enemy.
Self and dead robots are excluded; equal distances keep the first actor in
actor order. An empty choice clears the target. All three use the same selection
function. U05 keeps its existing fixed-enemy setup. Bullets keep the target
chosen when fired even if the shooter's choice changes later.

Sir Killalot uses the normal Actor's movement, wall avoidance, health, death and
three-second respawn. Death clears the tree's old execution; the revived actor
chooses afresh. R clears bullets, cooldowns, state-machine state and BT execution,
then restores all robots' health, position and velocity.

Space damages robot 1, Enter robot 2, and Backspace Sir Killalot by 25 HP per
press. Sir Killalot has a green tint and a name/HP label. Debug and Release
show Running status and cumulative root traversals in a collapsible ImGui window.
Retail keeps compact name/HP labels and omits the debug windows.

## Milestone 5 reading order

1. `Worlds/06_BehaviourTree/BT_WoundedCondition` and `BT_EnemyDetectedCondition`:
   the two read-only questions.
2. `RobotWarsBehaviourTree`: constructor child order, `ConfigureMovement`, and
   `GetActionName`; this owns the entire concrete tree.
3. `Controllers/DecisionMaking/CombatController::SelectNearestEnemy`: shared
   target selection before action execution.
4. `Worlds/05_RobotWars/GameWorld05_RobotWars`: optional bundle ownership;
   `Init`, `CreateRobots`, `Update`, `ResetBattle`, and labels connect gameplay.
5. `Worlds/06_BehaviourTree/GameWorld06_BehaviourTree.h`, then `Go.cpp`: simple
   assignment selection with no duplicated arena implementation.

## Milestone 4 verification

Debug and Release builds of `Game.sln` passed after review fixes. The leaf and
runtime checks also passed in both configurations. They cover stable conditions
resuming the action without another root traversal, changed conditions
interrupting before the old action updates, failed higher-priority questions
remaining monitored, nested Success and Failure continuation, reset, separate
tree state, and the actual Actor update and steering path.

The Visual Studio entries use the existing behaviour-tree and controller filters.
The runtime cannot be copied or moved, matching the nodes' single-execution
lifetime rule.

## Milestone 5 verification

Debug and Release builds of `Game.sln` passed with the Game project's `/W4`
and warnings-as-errors settings. Actual-tree checks passed in both configurations:
all three branches, persistent execution, the 39/40 HP boundary, healing before
a cooldown-ready shot, dead-target fallback, and fresh execution after reset.

The independent real-scene check reproduced a fatal bullet leaving Sir Killalot's
old action marked Running until the following frame. Update now resets execution
after bullet damage as well as before AI updates. The regression confirms an
idle tree in the fatal-hit frame, death, respawn, and clean battle reset.

The scene rendered and simulated 120 seconds at each of 60, 30, and 10 FPS.
Each run observed Heal, Combat, Wander, deaths, and respawns. All robot positions
and velocities stayed finite, speeds stayed within 180 units/s, HP stayed within
0-100, and every robot's radius stayed inside the arena. Target checks covered
nearest living enemy, self/dead exclusion, deterministic ties, no living enemy,
and flying bullets keeping their original targets after retargeting. The same
check initialized, updated, and rendered the original two-robot U05 scene.

## Complete reading guide

Start with the shared movement path, then follow the new tree from its smallest
parts to scene wiring. Read each header before its `.cpp`.

| Order | Files under `source/` | What to follow next |
| --- | --- | --- |
| 1 | `Actors/Actor`, `Controllers/Controller` | `CalculateSteering` handles the respawn timer, health and spawn position while dead, skipping AI. While alive it updates AI before querying desired velocity. `UpdateMovement` applies motion and keeps dead actors stopped. Then read node execution. |
| 2 | `DecisionMaking/BehaviourTree/BT_NodeResult`, `BT_Node` | `Tick` calls `Init` once while Running; `Reset` clears execution. Then read flow control. |
| 3 | `BT_CompositeNode`, `BT_Sequence`, `BT_Selector` | `AddChild` sets parent links; ordered children remember progress; `Continue` resumes after a completed child. Then read leaves. |
| 4 | `BT_LeafNodeCondition`, `BT_LeafNodeAction`, `BT_ControllerAction` | Conditions record their answers; actions remember themselves and call `Execute`. The adapter delegates to a borrowed controller. Then read the runtime. |
| 5 | `DecisionMaking/BehaviourTree/BehaviourTree` | `Run` rechecks questions, resumes the running action, or restarts when a result changes. Completion travels upward through parents. Then read Actor integration. |
| 6 | `Controllers/DecisionMaking/BehaviourTreeController` | `Update` runs the tree; `GetDesiredVelocity` reads the active leaf without updating it again. Then read the concrete questions. |
| 7 | `Worlds/06_BehaviourTree/BT_WoundedCondition`, `BT_EnemyDetectedCondition` | Read-only HP and detection checks. Then read construction. |
| 8 | `Worlds/06_BehaviourTree/RobotWarsBehaviourTree` | The constructor owns and links Heal, Combat, and Wander. `ConfigureMovement` uses the shared pool. Then read shared combat. |
| 9 | `Controllers/DecisionMaking/CombatController`, `Worlds/05_RobotWars/BulletSystem` | `SelectNearestEnemy` chooses before movement. `Update` fires only in Combat; bullets keep their original target. Then read scene order. |
| 10 | `Worlds/05_RobotWars/GameWorld05_RobotWars` | `Init` creates the optional tree before actors. `Update` selects targets, updates cooldowns/actors/bullets/pool, and clears dead BT execution. `ResetBattle` restores the battle. Then read selection. |
| 11 | `Worlds/06_BehaviourTree/GameWorld06_BehaviourTree.h`, `Go.cpp` | Selector 6 enables the shared three-robot scene; selector 5 keeps U05. |

The source tree is registered in `Local/Game.vcxproj` and its filters. Premake's
existing source glob already includes these files.

## Final requirement checklist

The agreed scope is U06's passing requirements. Optional VG resource waypoints
are outside this implementation.

| Requirement | Implementation and evidence |
| --- | --- |
| Third actor with the same behaviours | Sir Killalot is a normal Actor using BehaviourTreeController. The real scene exercises Heal, Combat and Wander alongside the Decision Tree and State Machine. |
| Selector and Sequence | RobotWarsBehaviourTree has one Selector and two Sequences with ordered condition/action children. Composite tests cover both flow rules and nested continuation. |
| Conditions are leaves | BT_WoundedCondition and BT_EnemyDetectedCondition inherit BT_LeafNodeCondition; no condition decorators are used. |
| Remember execution rather than traversing the root every frame | Stable-condition tests count one root traversal across repeated action updates. Changed failed or selected conditions interrupt before the old action updates. The ImGui tree window displays cumulative traversals. |
| Same healing priority | Below 40 HP selects Heal with no extra shot; 39/40 boundary checks pass. The real pool restores 10 HP/s and caps HP at 100. |
| Vision, chasing, shooting and wandering | All robots share nearest living target selection in U06. Tests cover self/dead filtering, equal-distance ordering, no enemy, retargeting, and bullets retaining their firing-time target. Existing chase/projectile tests pass. |
| Death and respawn remain in Actor | Three-second respawn restores health and spawn position. Dead actors skip movement and AI; fatal bullet damage clears BT execution before rendering. Battle reset clears previous execution and flying shots. |
| Smooth movement and walls | 120-second scenes at 60/30/10 FPS check finite position/velocity, speed limits, full-radius wall clearance and movement per frame; deaths and respawns are checked separately. |
| Warning level 4 and warnings as errors | Game and GameMain use Level4/TreatWarningAsError. Debug, Release and Retail builds pass; final standalone scene checks also compile with `/W4 /WX`. |
| Gameplay threads shut down correctly | No gameplay threads are created. |
| U05 still available | Selector 5 builds in Debug; the original two-robot scene initializes, updates and renders without the BT bundle. Selector 6 is restored for the final builds. |

## Final verification

The final real-engine scene harness runs in Debug, Release and Retail. Its checks
stay active in optimized builds. Each configuration runs 120 simulated seconds
at 60, 30 and 10 FPS and periodically renders the actual scene. It also checks
Space/Enter/Backspace, one hit per key press, R reset, names/HP/action labels,
BT debug status where enabled, gradual healing, target switching, death, respawn,
and same-frame fatal-hit cleanup. Retail retains gameplay labels and omits the
BT debug line and wall drawing.

The U05 automatic-healing, combat, state-transition and battle regressions passed.
Milestone 4's node, composite, leaf and persistent-runtime checks remain available.
Tests and build logs are under ignored `Temp/U06` and `Temp/DecisionTreeTests`;
there is no added production test harness. Visual interaction has not been
manually reviewed in this final pass; rendering and label contents were checked
by the real-engine harness. No submission package or remote push was made.


## ImGui inspection windows

Debug and Release use three movable, resizable windows in U06 and two in U05.
Their title bars collapse
the whole window; inspection sections also fold independently. The arena keeps
compact names and HP, while actions, states and diagnostics are in ImGui. The
fixed controls/BT text and built-in engine statistic overlays have been removed
for selectors 5 and 6. Retail keeps names and HP without debug UI.

- **Battle:** FPS, frame time, draw calls and bullet count; Pause, Step 1/60 s and
  Reset; collapsible controls/visuals and shared combat/pool settings. Walls are
  off initially, and both walls and arena labels can be toggled.
- **Robots:** action/state and HP bars for each robot; collapsible inspection
  shows wounded status, pool membership, respawn countdown, position, velocity,
  speed, selected enemy, distance/detection and firing cooldown. Damage/Heal
  buttons apply 25 HP to the selected robot. Dead robots cannot be healed.
- **Behaviour Tree (U06):** selected action, Running/Idle status, cumulative root
  traversals, condition results and the Selector/Sequence branch order.

Pause freezes actors, bullets, pool healing and respawn countdowns together.
Step advances one fixed 1/60-second frame while paused. UI keyboard capture
prevents focused ImGui navigation from applying the battle's damage/reset keys.
Read `GameWorld05_RobotWars::DrawDebugUI`, `GetRobotActionName` and
`GetRobotCombatController` for UI data flow. The combat controller's read-only
`GetEnemy` and `GetTimeUntilNextShot` expose existing state without changing AI.

The earlier final verification section records the milestone 6 version, before
this UI replacement. The new ImGui checks use compact labels and the windows
in place of the removed fixed BT line.

UI verification: Debug, Release and Retail builds passed with `/W4 /WX`. The
real-scene harness passed collapse/expand, pause/fixed-step, keyboard capture,
paused death and reset, compact labels, and the existing 60/30/10 FPS battles.
Rendered screenshots were inspected with 1600x900 and 1280x720 window settings,
including an expanded robot inspector. Default window bounds fit the viewport;
expanded details scroll. Checks remain under ignored `Temp/U06/ImGuiScene.cpp`
and `ImGuiPreview.cpp`.
