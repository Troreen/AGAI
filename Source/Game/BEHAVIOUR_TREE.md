# U06 Behaviour Tree

## Milestone progress

- 1: node results and execution lifecycle committed as `814e87a`.
- 2: ordered composites and completion continuation committed as `3fe4818`.
- 3: condition and action leaves committed as `2c02b2a`.
- 4: persistent runtime and steering integration complete; review and fixes verified.
- 5: three-robot scene and actual tree complete; review and fixes verified.
- 6: final verification and reading guide pending.

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
press. Sir Killalot has a green tint and a name/HP/action label. Debug and Release
also show its Running status and cumulative root-traversal count at the bottom
of the arena. Retail omits that debug line while retaining gameplay labels.

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
