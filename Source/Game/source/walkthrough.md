1. Load the mesh. Read the triangles from an FBX file, turn their ground positions into 2D positions, and fit them onto the screen.
2. Connect neighbouring triangles. If two triangles share a complete edge, the companion can move between them. These connections form the graph used for pathfinding.
3. Find the start and destination. Find which triangle contains the companion and which contains the target. If the target is outside the mesh, use the closest position on the mesh.
4. Find a route with A.* A* searches the connections and returns a sequence of triangles from start to goal.
5. Find the openings along that route. Each shared edge becomes a portal—an opening the companion must pass through.
6. Smooth the route with the funnel algorithm. Instead of visiting every triangle’s centre, find waypoints that pass through those openings with fewer turns.
7. Follow the waypoints. The companion’s controller steers it along the smoothed path. With smooth jazz. 
8. Keep movement on the mesh. Check the entire movement line each frame and stop before leaving the walkable surface or crossing a hole.

The overall flow is:

FBX triangles → connections → A triangle route → portals → smoothed waypoints → companion movement*