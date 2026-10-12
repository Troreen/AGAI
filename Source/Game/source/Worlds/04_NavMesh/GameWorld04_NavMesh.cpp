#include "GameWorld04_NavMesh.h"
#include "../../Controllers/Steering/PlayerController.h"
#include "../../Controllers/Navigation/PathFollowingController.h"

#include <tge/application.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/DX11.h>
#include <tge/settings/settings.h>
#include <tge/text/text.h>
#ifndef _RETAIL
#include <imgui/imgui.h>
#endif
#include <cstdio>

namespace
{
using CommonUtilities::Vector2f;
const char* navigationMeshFiles[] = {"Models/NavMesh_Models/navmesh.fbx",
                          "Models/NavMesh_Models/navmesh_debug.fbx"};

#ifndef _RETAIL
float PathLength(const std::vector<Vector2f>& somePathPoints)
{
    // Add the distances between each pair of points along the path.
    float totalPathLength = 0.f;
    // Count each segment from the previous point to the next one.
    for (std::size_t pathPointIndex = 1; pathPointIndex < somePathPoints.size(); ++pathPointIndex)
    {
        totalPathLength += somePathPoints[pathPointIndex - 1].Distance(somePathPoints[pathPointIndex]);
    }
    return totalPathLength;
}

void DrawPath(Tga::DebugDrawer& aDebugDrawer, const std::vector<Vector2f>& somePathPoints, const Tga::Color& aColor)
{
    // Draw a line for each path segment and a circle at each point.
    for (std::size_t pathPointIndex = 1; pathPointIndex < somePathPoints.size(); ++pathPointIndex)
    {
        aDebugDrawer.DrawLine(somePathPoints[pathPointIndex - 1].ToTga(), somePathPoints[pathPointIndex].ToTga(), aColor);
    }
    for (const Vector2f& pathPoint : somePathPoints)
    {
        aDebugDrawer.DrawCircle(pathPoint.ToTga(), 4.f, aColor);
    }
}
#endif
}

GameWorld04_NavMesh::GameWorld04_NavMesh() = default;
GameWorld04_NavMesh::~GameWorld04_NavMesh() = default;

Actor& GameWorld04_NavMesh::GetPlayer()
{
    // The player is created first, so it is actor 0.
    return myActorManager.GetActor(0);
}

Actor& GameWorld04_NavMesh::GetCompanion()
{
    // The companion is created second, so it is actor 1.
    return myActorManager.GetActor(1);
}

bool GameWorld04_NavMesh::LoadNavigationMesh()
{
    // Fit the selected mesh inside the screen, leaving a margin around it.
    const Tga::Vector2ui screenResolution = Tga::DX11::GetResolution();
    const Vector2f screenSize{static_cast<float>(screenResolution.x), static_cast<float>(screenResolution.y)};
    const std::string meshFilePath = Tga::Settings::ResolveEngineAssetPath(navigationMeshFiles[myNavMeshAssetIndex]);
    if (!myNavMesh.LoadFbx(meshFilePath, {100.f, 80.f}, screenSize - Vector2f{100.f, 80.f}))
    {
        std::printf("[U04] %s\n", myNavMesh.GetLoadError().c_str());
        return false;
    }
    // Clear the inspected triangle because the newly loaded mesh may have different triangles.
    mySelectedTriangleIndex = -1;
    std::printf("[U04] Loaded %zu triangle nodes from %s\n", myNavMesh.GetTriangles().size(), meshFilePath.c_str());
    return true;
}

void GameWorld04_NavMesh::Init(const CommonUtilities::InputHandler& aInput)
{
    // Load the walkable surface before creating the player and companion.
    LoadNavigationMesh();
    const Tga::Vector2ui screenResolution = Tga::DX11::GetResolution();
    const Vector2f playerPosition{static_cast<float>(screenResolution.x) * 0.35f, static_cast<float>(screenResolution.y) * 0.45f};
    Actor& player = myActorManager.CreateActor(playerPosition, "Sprites/human.png", std::make_unique<PlayerController>(playerPosition, aInput));
    myPlayerController = static_cast<PlayerController*>(player.GetController());
    player.SetMaxSpeed(300.f);
    player.SetMaxForce(1200.f);
    player.SetMass(0.15f);

    // Place the companion near the player, using the nearest point on the walkable surface.
    Vector2f companionPosition;
    int companionTriangleIndex = -1;
    myNavMesh.ClosestPoint(playerPosition - Vector2f{70.f, 30.f}, companionPosition, companionTriangleIndex);
    Actor& companion = myActorManager.CreateActor(companionPosition, "Sprites/robot1.png",
                                                  std::make_unique<PathFollowingController>(myNavMesh));
    myCompanionController = static_cast<PathFollowingController*>(companion.GetController());
    companion.SetMaxSpeed(200.f);
    companion.SetMaxForce(1200.f);
    companion.SetMass(0.15f);
    companion.SetColor({0.3f, 1.f, 0.8f, 1.f});
    // Give the two actors labels so their roles are visible on screen.
    const char* actorNames[] = {"Player", "Companion"};
    for (std::size_t actorIndex = 0; actorIndex < 2; ++actorIndex)
    {
        myActorLabels[actorIndex] = std::make_unique<Tga::Text>("Text/arial.ttf", Tga::FontSize_14);
        myActorLabels[actorIndex]->SetText(actorNames[actorIndex]);
    }
    myRequestedTarget = myPlayerController->GetTargetPosition();
    PlanCompanionPath();
}

void GameWorld04_NavMesh::PlanCompanionPath()
{
    Actor& companion = GetCompanion();
    // Plan from the companion to the latest clicked target and give its waypoints to the controller.
    myNavigationPath = myNavMesh.FindPath(companion.GetPosition(), myRequestedTarget);
    myCompanionController->SetPath(myNavigationPath.smoothPoints);
    // If planning fails, stop any movement left over from the previous route.
    if (!myNavigationPath.Succeeded())
    {
        companion.Stop();
        std::printf("[U04] Path failed: %s\n", myNavigationPath.error.c_str());
    }
}

void GameWorld04_NavMesh::Update(float aDeltaTime)
{
    // Let the player read input, then replan only when its requested target changes.
    GetPlayer().Update(aDeltaTime);
    const Vector2f requestedTargetPosition = myPlayerController->GetTargetPosition();
    if (requestedTargetPosition != myRequestedTarget)
    {
        myRequestedTarget = requestedTargetPosition;
        PlanCompanionPath();
    }
    Actor& companion = GetCompanion();
    if (myNavigationPath.Succeeded())
    {
        // The controller checks that nearby waypoints are reachable before moving directly onto them.
        // Choose a steering direction before saving the position we will move from.
        companion.CalculateSteering(aDeltaTime);
        const Vector2f positionBeforeMovement = companion.GetPosition();
        companion.UpdateMovement(aDeltaTime);
        const Vector2f proposedPosition = companion.GetPosition();
        // Check that this step stays on the mesh and leaves a clear route to the steering target.
        const Vector2f constrainedPosition = myNavMesh.ConstrainMovement(positionBeforeMovement, proposedPosition);
        const ControllerDebugInfo followTargetInfo = myCompanionController->GetDebugInfo();
        if (constrainedPosition != proposedPosition || (followTargetInfo.hasTarget && !myNavMesh.CanTraverseSegment(proposedPosition, followTargetInfo.targetPosition)))
        {
            // If momentum carries us across an edge or cuts a corner, undo that movement.
            // Take a checked step towards the steering target, then clear the old velocity.
            companion.SetPosition(positionBeforeMovement);
            const Vector2f correctedStepPosition = positionBeforeMovement + myCompanionController->GetDesiredVelocity(companion) * aDeltaTime;
            companion.SetPosition(myNavMesh.ConstrainMovement(positionBeforeMovement, correctedStepPosition));
            companion.Stop();
            ++myMovementCorrections;
        }
    }
    UpdateNavigationUI();
}

void GameWorld04_NavMesh::UpdateNavigationUI()
{
#ifndef _RETAIL
    // Show route details and controls for inspecting the navigation.
    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(mainViewport->ID);
    ImGui::SetNextWindowPos({mainViewport->WorkPos.x + 10.f, mainViewport->WorkPos.y + 10.f}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("U04 - NavMesh and Companion", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted("Left-click anywhere to set the player target.");
    ImGui::Text("Nodes: %zu | A* route: %zu | Funnel points: %zu",
                myNavMesh.GetTriangles().size(), myNavigationPath.nodes.size(), myNavigationPath.smoothPoints.size());
    ImGui::Text("Raw %.1f px | Follow %.1f px", PathLength(myNavigationPath.rawPoints), PathLength(myCompanionController->GetPath()));
    ImGui::Text("Start triangle: %d | Goal triangle: %d", myNavigationPath.startTriangle, myNavigationPath.goalTriangle);
    ImGui::Text("Requested: %.1f, %.1f", myRequestedTarget.x, myRequestedTarget.y);
    ImGui::Text("Resolved: %.1f, %.1f", myNavigationPath.resolvedTarget.x, myNavigationPath.resolvedTarget.y);
    ImGui::Text("Companion: %s | Boundary corrections: %u",
                myCompanionController->IsFinished() ? "arrived / stopped" : "following", myMovementCorrections);
    if (!myNavMesh.GetLoadError().empty())
    {
        ImGui::TextWrapped("Load: %s", myNavMesh.GetLoadError().c_str());
    }
    if (!myNavigationPath.error.empty())
    {
        ImGui::TextWrapped("Path: %s", myNavigationPath.error.c_str());
    }

    const char* navigationMeshAssetNames[] = {"Course navmesh", "Course debug navmesh"};
    ImGui::Combo("Asset", &myNavMeshAssetIndex, navigationMeshAssetNames, 2);
    if (ImGui::Button("Reload selected FBX") && LoadNavigationMesh())
    {
        // After reloading, move the companion onto the new mesh and plan again.
        Actor& companion = GetCompanion();
        Vector2f closestWalkablePosition;
        int companionTriangleIndex = -1;
        if (myNavMesh.ClosestPoint(companion.GetPosition(), closestWalkablePosition, companionTriangleIndex))
        {
            companion.SetPosition(closestWalkablePosition);
            companion.Stop();
        }
        PlanCompanionPath();
    }
    if (ImGui::CollapsingHeader("Debug drawing", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Navmesh triangles", &myShowNavMesh);
        ImGui::Checkbox("Triangle nodes", &myShowNodes);
        ImGui::Checkbox("Connections", &myShowConnections);
        ImGui::Checkbox("Raw A* path (orange)", &myShowRawPath);
        ImGui::Checkbox("Smoothed path (green)", &myShowSmoothPath);
        ImGui::Checkbox("Portals (left blue / right pink)", &myShowPortals);
        ImGui::Checkbox("Requested / resolved targets", &myShowTargets);
    }
    if (!myNavMesh.GetTriangles().empty() && ImGui::CollapsingHeader("Inspect node"))
    {
        ImGui::SliderInt("Node (-1 = none)", &mySelectedTriangleIndex, -1, static_cast<int>(myNavMesh.GetTriangles().size()) - 1);
        if (mySelectedTriangleIndex >= 0)
        {
            const Vector2f selectedTriangleCentre = myNavMesh.GetTriangles()[mySelectedTriangleIndex].centre;
            ImGui::Text("Centre: %.1f, %.1f", selectedTriangleCentre.x, selectedTriangleCentre.y);
            for (const CommonUtilities::PathfindingConnection& connection : myNavMesh.GetGraph()[mySelectedTriangleIndex].myConnections)
            {
                ImGui::Text("To %d: %.1f px", connection.myNodeIndex, connection.myCost);
            }
        }
    }
    ImGui::End();
#endif
}

void GameWorld04_NavMesh::Render()
{
#ifndef _RETAIL
    // Draw the walkable triangles and their connections, highlighting important triangles.
    Tga::DebugDrawer& debugDrawer = Tga::GraphicsEngine::GetInstance()->GetDebugDrawer();
    const std::vector<NavTriangle>& navTriangles = myNavMesh.GetTriangles();
    for (int triangleIndex = 0; triangleIndex < static_cast<int>(navTriangles.size()); ++triangleIndex)
    {
        const NavTriangle& triangle = navTriangles[triangleIndex];
        Tga::Color triangleColour{0.35f, 0.5f, 0.65f, 1.f};
        const bool isHighlightedTriangle = triangleIndex == myNavigationPath.startTriangle || triangleIndex == myNavigationPath.goalTriangle || triangleIndex == mySelectedTriangleIndex;
        if (triangleIndex == myNavigationPath.startTriangle) { triangleColour = {0.3f, 0.8f, 1.f, 1.f}; }
        if (triangleIndex == myNavigationPath.goalTriangle) { triangleColour = {1.f, 0.4f, 0.85f, 1.f}; }
        if (triangleIndex == mySelectedTriangleIndex) { triangleColour = {1.f, 1.f, 1.f, 1.f}; }
        if (myShowNavMesh || isHighlightedTriangle)
        {
            for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
            {
                debugDrawer.DrawLine(triangle.vertices[edgeIndex].ToTga(), triangle.vertices[(edgeIndex + 1) % 3].ToTga(), triangleColour);
            }
        }
        if (myShowNodes)
        {
            debugDrawer.DrawCircle(triangle.centre.ToTga(), 3.f, triangleColour);
        }
        if (myShowConnections || triangleIndex == mySelectedTriangleIndex)
        {
            for (const CommonUtilities::PathfindingConnection& connection : myNavMesh.GetGraph()[triangleIndex].myConnections)
            {
                debugDrawer.DrawLine(triangle.centre.ToTga(), navTriangles[connection.myNodeIndex].centre.ToTga(), triangleColour);
            }
        }
    }
    // Draw the planned route and its smoothed waypoints for comparison.
    if (myShowRawPath) { DrawPath(debugDrawer, myNavigationPath.rawPoints, {1.f, 0.65f, 0.15f, 1.f}); }
    if (myShowSmoothPath) { DrawPath(debugDrawer, myCompanionController->GetPath(), {0.25f, 1.f, 0.4f, 1.f}); }
    // Show the shared openings, with different colours for their left and right ends.
    if (myShowPortals)
    {
        for (const NavPortal& portal : myNavigationPath.portals)
        {
            debugDrawer.DrawLine(portal.left.ToTga(), portal.right.ToTga(), {0.85f, 0.85f, 1.f, 1.f});
            debugDrawer.DrawCircle(portal.left.ToTga(), 6.f, {0.2f, 0.6f, 1.f, 1.f});
            debugDrawer.DrawCircle(portal.right.ToTga(), 6.f, {1.f, 0.4f, 0.75f, 1.f});
        }
    }
    // Compare the clicked target with the actual destination on the mesh.
    if (myShowTargets)
    {
        debugDrawer.DrawCircle(myRequestedTarget.ToTga(), 12.f, {1.f, 0.8f, 0.2f, 1.f});
        if (myNavigationPath.goalTriangle >= 0)
        {
            debugDrawer.DrawCircle(myNavigationPath.resolvedTarget.ToTga(), 8.f, {0.25f, 1.f, 0.4f, 1.f});
            debugDrawer.DrawLine(myRequestedTarget.ToTga(), myNavigationPath.resolvedTarget.ToTga(), {0.7f, 0.7f, 0.7f, 1.f});
        }
    }
#endif
    // Draw the actors and place their labels beside them.
    myActorManager.Draw();
    for (std::size_t actorIndex = 0; actorIndex < 2; ++actorIndex)
    {
        const Actor& actor = myActorManager.GetActor(actorIndex);
        myActorLabels[actorIndex]->SetPosition((actor.GetPosition() + Vector2f{-30.f, 32.f}).ToTga());
        myActorLabels[actorIndex]->SetColor(actor.GetSpriteInstanceData().color);
        myActorLabels[actorIndex]->Render();
    }
}
