# U04: NavMesh and Companion

Build `GameMain` in `Game.sln` with Debug or Release, x64. Run from `Bin` so
the engine can find settings and assets. Left-click anywhere outside the debug
panel: the player arrives at that exact point, and the companion follows a
smoothed route to the nearest point on the navmesh.

## The flow to explain in the code review

1. `NavMesh_LoadFbx.h` adapts the supplied teacher loader. TGA's FBX importer
   reads the file and triangulates it, then each triangle becomes one node.
   The teacher's `(-Position[2], Position[0])` conversion maps the imported ground
   plane to screen X/Y. We uniformly fit the bounds into the actual drawable
   window, using pixel coordinates instead of the teacher's normalized-space helpers.
2. `Navigation/NavMesh.cpp::SetConnections` finds shared edges and adds a connection
   in both directions. Each cost is the distance between triangle centres.
   The main course asset has T-junctions, so collinear partial edges also connect
   through their common span. Triangles that touch only at a point do not connect.
3. `FindPath` first resolves the target. An interior point stays unchanged. For an
   exterior point, projecting onto every triangle edge and choosing the nearest
   candidate finds the closest position on the whole mesh. This includes holes,
   concave boundaries, and corners, not just the mesh's bounding rectangle.
4. The existing `CommonUtilities::AStar` searches those connections. Both the
   movement cost and heuristic use centre distance. `nodes` retains the route;
   `rawPoints` retains start, triangle centres, and the resolved goal for drawing.
5. `GetPortalBetweenNodes` orders each shared edge into left and right as seen
   travelling from the previous node to the next. These directed `portals` remain
   available for debugging independently of the path points.
6. `NavMesh_PerformFunnelling.h` adapts the supplied teacher funnel using
   `std::vector` and the project's vectors. `GetVectorRelation` uses the 2D cross
   product. Narrowing the funnel removes unnecessary centre points; when one side
   crosses the other, we add the opposite corner as a new apex. After choosing an
   apex, both sides restart at the next usable portal. A route inside one triangle
   goes directly from start to goal. The raw path is never overwritten.
7. `PathFollowingController` projects a predicted position onto the active path
   segment and seeks a point farther along it. The existing Arrive helper reduces
   speed near each corner and the goal. It finishes each necessary corner before
   taking the next segment, so it does not cut through holes. `Actor` still handles
   steering forces, acceleration, speed limits, rotation, and rendering.
8. `GameWorld::Update` checks the companion's complete movement segment after
   Actor integration. Triangle clipping finds how much of that segment is covered
   by the mesh, and movement stops at the first uncovered point. Checking only the
   endpoint would allow a large frame step to jump over a hole. The player gets
   no such restriction.

Navigation constrains the companion's **position/centre**. As discussed in F05,
the shortest funnel can touch a boundary or corner; the sprite itself can overlap
that boundary. This assignment does not add radius-based mesh erosion or physics.

## Files and ownership

`GameWorld` owns the navmesh, path result, and ActorManager. Each Actor owns its
controller using the project's existing ownership scheme. The companion controller
borrows the navmesh, which outlives the actors. A path result owns its vectors and
uses triangle indices, so it holds no pointers into a reloaded mesh.

Gameplay's small query API is `ClosestPoint`, `FindPath`, `FindTriangle`,
`CanTraverseSegment`, and `ConstrainMovement`. Gameplay does not implement A*
or funneling. A result keeps its requested/resolved targets, start/goal triangle
indices, raw node route, portals, smooth points, and failure text.

The engine importer is shared with TGE's model loading, so this loader leaves its
initialization under the existing engine lifecycle. It does not shut down the
importer while the engine is still using it. No Recast or editor navmesh system
is used by this game implementation.

## Debugging and assets

The supplied files are in
`EngineAssets/Models/NavMesh_Models/NavMesh_Models/`. The main asset imports as
166 triangles; the simpler debug asset imports as 10 triangles. The ImGui panel
lets you select and reload either file. A failed reload retains the previous
usable mesh and shows the error. A successful reload resolves the companion's
current position onto the new mesh and calculates a fresh path.

The panel provides independent toggles for triangles, nodes, connections, raw
path, smoothed path, portals, and targets. Select a node to highlight its triangle
and connections and read their costs. Start triangles are blue, goal triangles
pink, raw paths orange, and smoothed paths green. Portal endpoints show left in
blue and right in pink. The requested target is yellow; the resolved target is
green. Lengths and boundary-correction counts help distinguish path and movement
problems. Debug drawing is available in Debug and Release, as in the existing
project; Retail omits the debug UI/drawing.

The main FBX contains a separate 12-triangle island. There is no walkable link to
it. If the globally closest target lies there while the companion is on the main
region, A* reports no connected route and the companion stops. The requested and
resolved targets remain visible. We do not teleport or invent a connection across
empty space. The smaller debug mesh is fully connected.

## Checks performed

- Debug and Release GameMain builds with `/W4` and `/WX`.
- All centre-to-centre queries on both assets: 23,960 reachable routes; every
  smoothed segment stayed on the mesh and no smoothed route was longer than raw.
- Off-mesh screen samples resolved to valid positions, including boundary targets.
- Real TGE renderer and Actor/controller integration: the player reached interior
  and off-mesh targets; 171 reachable companion routes completed at 60, 30, and
  10 Hz without off-mesh movement or stalls. Target changes while moving were checked.
- GPU frame capture confirmed the navmesh, actors, targets, and both path drawings.
- The reusable event delivery/unregistering and polling cache refresh behaviour
  passed small temporary C++ checks. No separate test game or headless mode was added.

The rules come from [the U04 assignment](../../Doc/U04_NavMesh_och_Companion.pdf).
[F05 Navigation](../../Doc/F05_Navigation.pdf) explains the graph, funnel, and
path-following approach. Reusable events/polling are documented in
[CommonUtilities](../../CommonUtilities/AI_UTILITIES.md).
