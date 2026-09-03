# Steering controllers

This project uses steering controllers to decide an actor's desired movement. The actor converts the chosen steering force into acceleration and movement, subject to its mass, maximum force, and maximum speed.

## Current architecture

Each active controller has two parts:

1. A **primary behavior** creates the normal steering force: seek, arrive, or wander.
2. A shared **containment behavior** checks the actor's predicted position against `ITraversalBounds`.

While the actor is close to leaving the level, containment is added to the primary force. Once the actor is already outside valid space, containment takes priority until it is back inside. This prevents normal behavior from fighting recovery.

`GameWorld` currently supplies `RectTraversalBounds`, based on the render area. Controllers only depend on `ITraversalBounds`, so a larger project can provide another implementation for polygons, tile maps, or a navmesh.

## Config data

Every controller starts with `ControllerData`:

| Value | Meaning | Typical tuning direction |
| --- | --- | --- |
| `behaviorWeight` | Strength of the controller's normal steering force. | Raise for a more decisive primary behavior. |
| `containmentWeight` | Strength of edge correction before the actor crosses a bound. | Raise to make actors turn away earlier/sharper. |
| `boundaryLookAhead` | Seconds of velocity projected when checking bounds. | Raise for faster, smoother anticipation; lower for later turns. |
| `boundaryClearance` | Extra empty space between the actor's radius and the level edge. | Raise to keep actors farther from the edge. |
| `useContainment` | Enables bounds containment for this controller. | Disable only when the actor should be able to leave the traversal area. |

`TargetControllerData` extends it for seeker and arrive:

| Value | Meaning |
| --- | --- |
| `targetReachedRadius` | Seeker chooses a new target once this close to the current target. |
| `randomSeed` | Makes random targets repeatable. Give actors different seeds so they do not follow identical paths. |

`ArriveControllerData` additionally provides:

| Value | Meaning |
| --- | --- |
| `slowingRadius` | Desired speed scales down linearly inside this radius. |
| `targetChangeInterval` | How often arrive selects a new random target. |

`WanderControllerData` additionally provides:

| Value | Meaning |
| --- | --- |
| `circleRadius` | Size of the circle containing the wander target. Larger produces broader drift. |
| `circleDistance` | Distance of the wander circle ahead of the actor. Larger produces more forward motion. |
| `maxTurnRate` | Maximum random angular turn rate in radians per second. Larger produces more erratic motion. |
| `directionChangeInterval` | Time between random turn-rate samples. Smaller changes direction more frequently. |
| `randomSeed` | Makes a wanderer's random path repeatable. |

## Actor movement values

| Value | Meaning |
| --- | --- |
| `maxSpeed` | Caps final actor velocity. |
| `maxForce` | Caps steering force, and therefore acceleration/turning. |
| `mass` | Reduces acceleration from a given force when increased. |
| `radius` | Actor occupancy radius. Bounds are inset by `radius + boundaryClearance`. |

## Random target safety

Seek and arrive random targets are passed through `GetNearestValidPoint` using the requesting actor's radius and clearance. A target cannot therefore be generated in an unreachable corner of the bounds.

## Debug drawing

In non-retail builds, `SteeringDebugRenderer` displays:

| Colour | Meaning |
| --- | --- |
| White circle | Physical actor radius. |
| Grey circle | Actor radius plus configured boundary clearance. |
| Cyan arrow and circle | Look-ahead position when it remains valid. |
| Orange arrow and circle | Look-ahead position that requires containment correction. |
| Magenta circle and arrow | Nearest valid point and recovery direction. |
| Green arrow | Desired velocity. |
| Red arrow | Previous steering force actually applied. |

## Adding a bounds type

Implement `ITraversalBounds` for the new world representation. `GetNearestValidPoint` must project a point to valid navigable space for a supplied inset. `GetRecoveryDirection` returns the direction to steer from the current position when the predicted position is invalid. Pass the shared bounds object to each controller with `SetTraversalBounds`.
