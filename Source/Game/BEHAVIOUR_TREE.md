# U06 Behaviour Tree

## Milestone progress

- 1: node results and execution lifecycle committed as `814e87a`.
- 2: ordered composites and completion continuation committed as `3fe4818`.
- 3: condition and action leaves committed as `2c02b2a`.
- 4: persistent runtime and steering integration complete; review and fixes verified.
- 5: three-robot scene and actual tree pending.
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
