# U04 navigation implementation

The navigation code loads a triangle mesh, plans a route to the player's requested
target, and moves the companion along the resulting waypoints. This guide follows
that implementation from loading through per-frame movement.

## Data and ownership

[NavMesh.h](source/Navigation/NavMesh.h) defines the navigation data:

- `NavTriangle` stores three 2D vertices and their centre.
- `NavPortal` stores a shared edge's left and right endpoints, ordered for a
  particular direction of travel.
- `NavMesh` owns the triangles, graph connections, and load error.
- `NavigationPath` stores one planning result.

The triangle and graph arrays use matching indices: graph node `i` represents
triangle `i`. A graph connection stores a neighbouring triangle's index and the
distance between their centres.

The path result keeps each planning stage available separately:

| Field | Use in this implementation |
| --- | --- |
| `requestedTarget` | The position requested by the player controller. |
| `resolvedTarget` | The destination after resolving it onto the mesh. |
| `startTriangle` / `goalTriangle` | Indices used as the A* start and goal. |
| `nodes` | A*'s ordered triangle indices, including start and goal. |
| `rawPoints` | Start position, route triangle centres, and resolved destination. |
| `portals` | Directed shared edges between consecutive route triangles. |
| `smoothPoints` | Funnel waypoints passed to the path-following controller. |
| `error` | The reason planning failed, if any. |

`Succeeded()` requires an empty error string and a nonempty `smoothPoints` array.

`GameWorld04_NavMesh` owns the mesh, current path result, and ActorManager.
The companion Actor owns its `PathFollowingController`. That controller borrows
the mesh through a const reference, so the mesh must outlive the controller.
Path results and controllers own their point arrays; triangle references in a
path result are indices rather than pointers.

## Loading and preparing the mesh

[NavMesh_LoadFbx.h](source/Navigation/NavMesh_LoadFbx.h) implements `LoadFbx`.
It is included by [NavMesh.cpp](source/Navigation/NavMesh.cpp), where its geometry
helpers are defined.

The loader builds a temporary `NavMesh` before replacing the current data:

1. Initialize the shared TGA FBX importer and import with triangulation enabled.
2. Read each polygon into a `NavTriangle`. Reject non-triangles, invalid vertex
   indices, and non-finite positions.
3. Convert imported positions to 2D using `{-Position[2], Position[0]}`,
   giving the `(-Z, X)` ground-plane mapping.
4. Calculate the mesh bounds and fit them into the supplied screen-space bounds.
   Use the smaller of the X and Y scale factors for both axes, then centre the
   result in the available area.
5. Reject degenerate triangles, reverse clockwise vertex order, and calculate
   each centre as the average of its three vertices.
6. Build graph connections, then move the completed data into the current mesh.

A failed load sets `myLoadError` and leaves the previous mesh data intact.
A successful load clears that error. The importer remains initialized because
it is shared with engine model loading.

### Building connections and portals

`SetConnections` checks every triangle pair once. For each pair,
`GetPortalBetweenNodes` compares their edges by position, rather than imported
vertex indices. This handles shared geometry split across FBX chunks.

The edge comparison requires collinear edges, triangle centres on opposite sides
of the edge, and an overlap longer than `pointTolerance`. It accepts partial
overlaps for the course mesh's T-junctions and rejects point-only contact.

For a connected pair, `SetConnections` adds connections in both directions.
Both costs use the distance between the triangle centres.

`GetPortalBetweenNodes` also orders the overlap endpoints. It uses the direction
from the source triangle's centre to the destination triangle's centre and a
cross product to determine left and right. Calling it in the reverse direction
therefore gives the portal the opposite orientation.

## Planning a path

`NavMesh::FindPath` in [NavMesh.cpp](source/Navigation/NavMesh.cpp) performs
the complete planning operation and returns a `NavigationPath`.

### Resolve the start and destination

`FindTriangle` scans the triangle array and calls `Contains`. Because loading
ensures counterclockwise vertex order, `Contains` can test the point against
each edge's inside half-plane. Small errors near an edge are accepted using
`pointTolerance`. The result is a triangle index, or `-1`.

`FindPath` uses this query for the start. It does not relocate an off-mesh start;
that makes planning fail.

For the destination, `ClosestPoint` first calls `FindTriangle`. An interior
target stays unchanged. Otherwise, it projects the target onto every triangle
edge, clamps each projection to the edge endpoints, and keeps the candidate
with the smallest squared distance. The result supplies both `resolvedTarget`
and `goalTriangle`.

This chooses the closest point on the whole mesh, regardless of connectivity.
If that point lies on a disconnected island, the following A* stage can fail.

### Search the graph

`FindPath` calls [CommonUtilities::AStar](../../CommonUtilities/include/Pathfinding/AStar.hpp)
with `myGraph`, `startTriangle`, and `goalTriangle`. Its heuristic lambda
returns the distance between the supplied triangles' centres, matching the
centre-distance units used by connection costs.

A* returns the ordered node indices. An empty result sets a path error and ends
planning. A successful result is saved in `nodes`; `rawPoints` is then built
from the start position, each route triangle's centre, and the resolved target.

The search minimizes graph costs. The final waypoint path is smoothed within
that selected route; the implementation does not search every possible corridor
for a globally shortest geometric path.

### Build portals and funnel them

For each consecutive pair of route nodes, `FindPath` calls
`GetPortalBetweenNodes` and appends the directed portal to `portals`.
A missing shared edge sets an error and ends planning.

`PerformFunnelling` in
[NavMesh_PerformFunnelling.h](source/Navigation/NavMesh_PerformFunnelling.h)
then produces `smoothPoints`:

- Start with the start position as the apex and the first portal as the
  left/right limits. Append a zero-width goal portal to process the destination.
- For each subsequent portal, test its sides against the existing limits using
  `GetVectorRelation`, which checks the sign of the 2D cross product.
- Tighten a limit when the new side narrows the funnel without crossing the
  opposite side.
- When a side crosses the opposite limit, append the opposite endpoint as a
  waypoint and use it as the new apex. Restart at the next portal whose
  corresponding endpoint differs from that apex.
- Append the goal if the final point is not already within `pointTolerance`.

With no portals, the result is simply start and goal.

Finally, `FindPath` checks every smoothed segment with `CanTraverseSegment`.
An invalid segment sets an error and clears `smoothPoints`. Raw points and
portals remain available for inspection.

## Following the path

[PathFollowingController.cpp](source/Controllers/Navigation/PathFollowingController.cpp)
copies `smoothPoints` in `SetPath`, without changing their corners.
`myNextPoint` normally starts at 1 because point 0 is the start position.
A single-point path starts at 0; an empty path is already finished.

### Update and waypoint completion

`Update` first checks whether the Actor is within `arrivalDistance`
(1.5 pixels) of the active waypoint and can reach it directly.
If so, it places the Actor exactly there, stops it, and increments
`myNextPoint`. It keeps the steering target at the Actor's position for
that frame, then returns.

This lets boundary corners finish before movement turns onto the next segment.
Otherwise, a finished path stops the Actor, and an unfinished path calls
`ChooseFollowTarget`.

### Select progress and a look-ahead target

`ChooseFollowTarget` initially selects the active waypoint with Arrive enabled.
That remains the fallback if a look-ahead shortcut is blocked.

To determine progress, it scans the remaining segments and computes a clamped
projection for each:

```cpp
amount = clamp(dot(position - start, segment) / lengthSquared, 0, 1);
projection = start + segment * amount;
```

This is the calculation used in the code, with a zero-length guard for repeated
waypoints. Candidates are compared using squared distance to the Actor.
A candidate is accepted only if both its projection and its segment endpoint
are directly traversable. The nearest accepted candidate updates the active
segment through `myNextPoint`. Equal distances favour the later candidate.

Starting from that projection, the controller walks `lookAheadDistance`
(currently 45 pixels) along the remaining segments. It subtracts the distance
used on each segment and carries the remainder onto the next, stopping at the
destination if the path ends first.

This calculation chooses an aiming point; it does not advance `myNextPoint`
to that aiming point's segment. Progress comes from the projection search.

The controller uses the look-ahead point only if `CanTraverseSegment` accepts
the direct shortcut from the Actor to it. Otherwise, it keeps the active
waypoint as its target.

### Produce desired velocity

`GetDesiredVelocity` returns zero for a finished path. Otherwise it calls:

- `ArriveDesiredVelocity` when using the fallback waypoint or when the
  look-ahead point is on the final segment.
- `SeekDesiredVelocity` for other accepted look-ahead targets.

Arrive uses `lookAheadDistance` as its slowing distance.
The controller supplies desired velocity; `Actor` computes steering, applies
force and speed limits, and integrates movement. `GetDebugInfo` exposes the
current steering target, which can lie between waypoints.

## Checking movement against the mesh

`CanTraverseSegment` and `ConstrainMovement` both use
`NavMesh::TraversableFraction`. This checks the entire movement segment, so
a step cannot cross a hole just because its endpoint is on the mesh.

`TraversableFraction` clips the segment against each triangle. Each intersection
produces an entry/exit interval between 0 and 1 along the movement.
It sorts those intervals and merges coverage from the start until the first gap.
The resulting fraction indicates how much of the requested movement is covered.

`CanTraverseSegment` requires the start to be on the mesh and coverage to reach 1.
`ConstrainMovement` returns the proposed endpoint for complete coverage.
Otherwise, it stops slightly before the uncovered part, using `pointTolerance`
to leave room for floating-point error.

The constraint applies to the Actor's centre. It does not account for sprite
size or erode the mesh for the Actor's radius.

## Connecting planning and movement in the world

[GameWorld04_NavMesh.cpp](source/Worlds/04_NavMesh/GameWorld04_NavMesh.cpp)
coordinates navigation:

1. Initialization loads the mesh and uses `ClosestPoint` to place the companion.
2. Each update reads the player controller's requested target. If it changed,
   `PlanCompanionPath` plans from the companion's current position and gives
   `smoothPoints` to its controller. A failed plan stops the companion.
3. For a successful path, save the companion's position and call its
   `Actor::Update`.
4. Constrain the segment from the saved position to the proposed position.
   If corrected, apply the constrained position, stop the companion, and
   increment `myMovementCorrections`.

Planning happens when the requested target changes or the mesh is successfully
reloaded, rather than on every movement frame. A successful reload resolves
the companion's current position onto the new mesh before replanning.
The player is updated separately and receives no navmesh movement constraint.

The world's rendering code draws `rawPoints` in orange, the controller's copied
waypoints in green, and the stored portals and requested/resolved targets.
These arrays are retained so planning stages can be inspected independently.
