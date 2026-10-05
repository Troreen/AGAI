# Applied Game AI

This repository contains coursework and experiments for the **Applied Game AI** course, built on the TGA engine (TGE).

The current game project implements **U04 - NavMesh and Companion**:

- An unrestricted mouse-target player using Arrive steering.
- A companion using triangle-node A*, the supplied course funnel, and path following.
- File-loaded course FBX navmeshes and debug views for paths, portals, and connections.

See [Navigation](Source/Game/NAVIGATION.md) for the implementation flow, controls,
asset details, and verification. Earlier steering behaviours remain available in
the [steering system](Source/Game/STEERING.md). Reusable event and polling code
now lives in [CommonUtilities](CommonUtilities/AI_UTILITIES.md).

## Running the project

1. Open `Game.sln` in Visual Studio 2025.
2. Select the `Debug | x64` configuration.
3. Build `GameMain` and run it with `Bin` as the working directory.

If project files need to be regenerated, run `generate_game.bat` from the repository root.

## Backlog

- [ ] Add obstacle avoidance steering.
- [ ] Add flocking behaviors: separation, alignment, and cohesion.
- [ ] Add polygon, tile-map, or navmesh implementations of `ITraversalBounds`.
- [ ] Draw traversal-bound outlines in the steering debug renderer.
- [ ] Move the controller-tuning UI out of `GameWorld`.
- [ ] Save controller configuration data per level or actor archetype.
