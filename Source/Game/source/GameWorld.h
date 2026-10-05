#pragma once

#include "Actor.h"
#include "Managers/ActorManager.h"
#include "Navigation/NavMesh.h"
#include <array>
#include <memory>

namespace CommonUtilities { class InputHandler; }
namespace Tga { class Text; }
class PlayerController;
class PathFollowingController;

// Owns the U04 scene. Navigation -> A* -> funnel -> steering is visible in Update.
class GameWorld
{
public:
    GameWorld();
    ~GameWorld();
    void Init(const CommonUtilities::InputHandler& aInput);
    void Update(float aTimeDelta);
    void Render();
    Actor& GetPlayer();

private:
    bool LoadNavigationMesh();
    void PlanCompanionPath();
    void UpdateNavigationUI();

    // Data outlives the actors/controllers that borrow it.
    NavMesh myNavMesh;
    NavigationPath myNavigationPath;
    ActorManager myActorManager;
    PlayerController* myPlayerController = nullptr; // Owned by the player Actor.
    PathFollowingController* myCompanionController = nullptr; // Owned by the companion Actor.
    CommonUtilities::Vector2f myRequestedTarget;
    std::array<std::unique_ptr<Tga::Text>, 2> myLabels;

    bool myShowNavMesh = true;
    bool myShowNodes = true;
    bool myShowConnections = false;
    bool myShowRawPath = true;
    bool myShowSmoothPath = true;
    bool myShowPortals = false;
    bool myShowTargets = true;
    int mySelectedNode = -1;
    int myNavMeshAsset = 0;
    unsigned int myMovementCorrections = 0;
};
