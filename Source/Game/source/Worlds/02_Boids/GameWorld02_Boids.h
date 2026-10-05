#pragma once

#include "../GameWorld.h"

#include <memory>
#include <span>
#include <Spatial/Grid2D.hpp>
#include "../../Actors/ActorManager.h"
#include "../../Controllers/Flocking/FlockingSettings.h"
#include "../../Controllers/Steering/ObstacleAvoidance.h"

class RectTraversalBounds;

class GameWorld02_Boids final : public GameWorld
{
public:
    GameWorld02_Boids();
    ~GameWorld02_Boids() override;

    void Init(const CommonUtilities::InputHandler& aInput) override;
    void Update(float aTimeDelta) override;
    void Render() override;
    // Fill caller-owned scratch storage; the returned view lasts until that storage is reused.
    // Query before movement, while actor positions still match the grid.
    std::span<const Actor* const> FindNeighbours(const Actor& aActor, float aRadius, std::span<const Actor*> aStorage) const;
private:
    void UpdateDebugUI();
    void RebuildFlockingGrid();
    void DrawFlockingDebug();
    void DrawObstacleDebug();

    // Number of boid to spawn; also reserves enough stack space for every possible neighbour.
    static constexpr std::size_t actorCount = 100;
    // Show only the red boid's neighbourhood to keep the debug view readable.
    bool myShowDetectionRadius = false;
    bool myShowCohesion = false;
    bool myShowAlignment = false;
    bool myShowSeparation = false;
    // Draw the test obstacles and the old containment rectangle independently of ray debugging.
    bool myShowObstacles = true;
    bool myShowTraversalBounds = true;
    bool myShowAvoidanceRays = false;
    // Show other boid's rays too; otherwise only the red boid is inspected.
    bool myShowAllAvoidanceRays = false;
    // Explicitly opt in to the old edge-steering behavior on the wander controllers.
    bool myUseContainment = false;
    ObstacleAvoidanceSettings myObstacleSettings;
    std::vector<CircleObstacle> myObstacles;
    FlockingSettings myFlockingSettings;
    CommonUtilities::Grid2D myFlockingGrid;
    std::shared_ptr<RectTraversalBounds> myTraversalBounds;
    // Actors borrow the settings and obstacles above; destroy actors first.
    ActorManager myActorManager;
};
