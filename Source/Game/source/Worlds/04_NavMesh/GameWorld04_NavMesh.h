#pragma once

#include "../GameWorld.h"

#include "../../Actors/Actor.h"
#include "../../Actors/ActorManager.h"
#include "../../Navigation/NavMesh.h"
#include <array>
#include <memory>

namespace CommonUtilities { class InputHandler; }
namespace Tga { class Text; }
class PlayerController;
class PathFollowingController;

// GameWorld coordinates the navigation system. It initializes the navmesh and actors, handles target changes, and requests paths.
class GameWorld04_NavMesh final : public GameWorld
{
public:
    GameWorld04_NavMesh();
    ~GameWorld04_NavMesh() override;
    void Init(const CommonUtilities::InputHandler& aInput) override;
    void Update(float aDeltaTime) override;
    void Render() override;
    Actor& GetPlayer();

private:
    bool LoadNavigationMesh();
    void PlanCompanionPath();
    void UpdateNavigationUI();
    Actor& GetCompanion();

    // Data outlives the actors/controllers that borrow it.
    NavMesh myNavMesh;
    NavigationPath myNavigationPath;
    ActorManager myActorManager;
    PlayerController* myPlayerController = nullptr; // Owned by the player Actor.
    PathFollowingController* myCompanionController = nullptr; // Owned by the companion Actor.
    CommonUtilities::Vector2f myRequestedTarget;
    std::array<std::unique_ptr<Tga::Text>, 2> myActorLabels;

    bool myShowNavMesh = true;
    bool myShowNodes = true;
    bool myShowConnections = false;
    bool myShowRawPath = true;
    bool myShowSmoothPath = true;
    bool myShowPortals = false;
    bool myShowTargets = true;
    int mySelectedTriangleIndex = -1;
    int myNavMeshAssetIndex = 0;
    unsigned int myMovementCorrections = 0;
};
