# Applied Game AI

This repository contains coursework and experiments for the **Applied Game AI** course, built on the TGA engine (TGE).

The current game project demonstrates 2D steering behaviors:

- Seek
- Arrive
- Wander
- Predictive traversal-bounds containment

See [Steering system documentation](Source/Game/STEERING.md) for the current controller architecture, tuning values, bounds system, and debug overlays.

## Running the project

1. Open `Game.sln` in Visual Studio 2025.
2. Select the `Debug | x64` configuration.
3. Build and run the game project.

If project files need to be regenerated, run `generate_game.bat` from the repository root.

## Backlog

- [ ] Add obstacle avoidance steering.
- [ ] Add flocking behaviors: separation, alignment, and cohesion.
- [ ] Add polygon, tile-map, or navmesh implementations of `ITraversalBounds`.
- [ ] Draw traversal-bound outlines in the steering debug renderer.
- [ ] Move the controller-tuning UI out of `GameWorld`.
- [ ] Save controller configuration data per level or actor archetype.
