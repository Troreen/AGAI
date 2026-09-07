#pragma once
#include <memory>
#include <vector>
#include <tge/math/vector.h>
#include "ActorManager.h"
#include "ObstacleAvoidance.h"
#include "TraversalBounds.h"
#include "FreeFlyCameraController.h"

class Boid3D;
namespace Tga { class Camera; class ModelInstance; }

class GameWorld
{
public:
    GameWorld(); ~GameWorld();
    void Init(); void Update(float aTimeDelta); void Render();
private:
    ActorManager myActorManager;
    std::unique_ptr<RectTraversalBounds3D> myTraversalBounds3D;
    std::vector<std::unique_ptr<Boid3D>> myBoids3D;
    std::vector<SphericalObstacle<Tga::Vector3f>> myObstacles;
    std::vector<std::shared_ptr<Tga::ModelInstance>> myObstacleModels;
    std::shared_ptr<Tga::ModelInstance> myGround;
    std::shared_ptr<Tga::Camera> myCamera;
    std::unique_ptr<Tga::InputManager> myInputManager;
    FreeFlyCameraController myCameraController;
};
