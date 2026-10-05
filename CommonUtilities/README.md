# CommonUtilities folder guide

Public headers live in `include`, grouped by purpose. The include root remains
`CommonUtilities/include`; include the category as part of the header name:

```cpp
#include <Math/Vector2.hpp>
#include <Input/InputHandler.h>
#include <Pathfinding/AStar.hpp>
#include <Events/EventManager.hpp>
```

| Folder in include | Contents |
| --- | --- |
| Algorithms | Sudoku solver |
| Containers | Heap, queue, stack, linked list, and binary search tree |
| Events | Event declarations, event delivery, and per-frame polling cache |
| Geometry | Shapes, rays, lines, planes, and intersection helpers |
| Graphics | Camera3D |
| Input | InputHandler and keyboard/mouse key enum |
| Math | Vectors, matrices, quaternion, transform, and math helpers |
| Pathfinding | A*, Dijkstra, and shared graph/path types |
| Spatial | Grids, grid raytracing, KD-tree, and quadtrees |
| Time | Timer |

Implementation files live in `source/Input` and `source/Time`. The existing
library and precompiled-header scaffolding stays in `source`, away from public
headers. Game compiles `source/Input/InputHandler.cpp` directly; the other utility
implementations are available for projects that use them.

See [AI_UTILITIES.md](AI_UTILITIES.md) and [EVENTS.md](EVENTS.md) for events and polling.

After the folder cleanup, a temporary C++20 compilation checked all public headers
and the moved Timer/library implementation files. GameMain builds also compile
the moved InputHandler implementation.
