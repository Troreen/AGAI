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
const char* meshFiles[] = {"Models/NavMesh_Models/navmesh.fbx",
                          "Models/NavMesh_Models/navmesh_debug.fbx"};

#ifndef _RETAIL
float PathLength(const std::vector<Vector2f>& somePoints)
{
    float length = 0.f;
    for (std::size_t index = 1; index < somePoints.size(); ++index)
    {
        length += somePoints[index - 1].Distance(somePoints[index]);
    }
    return length;
}

void DrawPath(Tga::DebugDrawer& aDrawer, const std::vector<Vector2f>& somePoints, const Tga::Color& aColor)
{
    for (std::size_t index = 1; index < somePoints.size(); ++index)
    {
        aDrawer.DrawLine(somePoints[index - 1].ToTga(), somePoints[index].ToTga(), aColor);
    }
    for (const Vector2f& point : somePoints)
    {
        aDrawer.DrawCircle(point.ToTga(), 4.f, aColor);
    }
}
#endif
}

GameWorld04_NavMesh::GameWorld04_NavMesh() = default;
GameWorld04_NavMesh::~GameWorld04_NavMesh() = default;

Actor& GameWorld04_NavMesh::GetPlayer()
{
    return myActorManager.GetActor(0);
}

bool GameWorld04_NavMesh::LoadNavigationMesh()
{
    const Tga::Vector2ui size = Tga::DX11::GetResolution();
    const Vector2f resolution{static_cast<float>(size.x), static_cast<float>(size.y)};
    const std::string file = Tga::Settings::ResolveEngineAssetPath(meshFiles[myNavMeshAsset]);
    if (!myNavMesh.LoadFbx(file, {100.f, 80.f}, resolution - Vector2f{100.f, 80.f}))
    {
        std::printf("[U04] %s\n", myNavMesh.GetLoadError().c_str());
        return false;
    }
    mySelectedNode = -1;
    std::printf("[U04] Loaded %zu triangle nodes from %s\n", myNavMesh.GetTriangles().size(), file.c_str());
    return true;
}

void GameWorld04_NavMesh::Init(const CommonUtilities::InputHandler& aInput)
{
    LoadNavigationMesh();
    const Tga::Vector2ui size = Tga::DX11::GetResolution();
    const Vector2f playerPosition{static_cast<float>(size.x) * 0.35f, static_cast<float>(size.y) * 0.45f};
    Actor& player = myActorManager.CreateActor(playerPosition, "Sprites/human.png",
                                              std::make_unique<PlayerController>(playerPosition, aInput));
    myPlayerController = static_cast<PlayerController*>(player.GetController());
    player.SetMaxSpeed(300.f);
    player.SetMaxForce(1200.f);
    player.SetMass(0.15f);

    Vector2f companionPosition;
    int triangle = -1;
    myNavMesh.ClosestPoint(playerPosition - Vector2f{70.f, 30.f}, companionPosition, triangle);
    Actor& companion = myActorManager.CreateActor(companionPosition, "Sprites/robot1.png",
                                                  std::make_unique<PathFollowingController>(myNavMesh));
    myCompanionController = static_cast<PathFollowingController*>(companion.GetController());
    companion.SetMaxSpeed(200.f);
    companion.SetMaxForce(1200.f);
    companion.SetMass(0.15f);
    companion.SetColor({0.3f, 1.f, 0.8f, 1.f});
    const char* names[] = {"Player", "Companion"};
    for (std::size_t index = 0; index < 2; ++index)
    {
        myLabels[index] = std::make_unique<Tga::Text>("Text/arial.ttf", Tga::FontSize_14);
        myLabels[index]->SetText(names[index]);
    }
    myRequestedTarget = myPlayerController->GetTargetPosition();
    PlanCompanionPath();
}

void GameWorld04_NavMesh::PlanCompanionPath()
{
    Actor& companion = myActorManager.GetActor(1);
    myNavigationPath = myNavMesh.FindPath(companion.GetPosition(), myRequestedTarget);
    myCompanionController->SetPath(myNavigationPath.smoothPoints);
    if (!myNavigationPath.Succeeded())
    {
        companion.Stop();
        std::printf("[U04] Path failed: %s\n", myNavigationPath.error.c_str());
    }
}

void GameWorld04_NavMesh::Update(float aTimeDelta)
{
    GetPlayer().Update(aTimeDelta);
    const Vector2f requested = myPlayerController->GetTargetPosition();
    if (requested != myRequestedTarget)
    {
        myRequestedTarget = requested;
        PlanCompanionPath();
    }
    Actor& companion = myActorManager.GetActor(1);
    if (myNavigationPath.Succeeded())
    {
        const Vector2f before = companion.GetPosition();
        companion.Update(aTimeDelta);
        const Vector2f proposed = companion.GetPosition();
        const Vector2f constrained = myNavMesh.ConstrainMovement(before, proposed);
        if (constrained != proposed)
        {
            // Steering is smooth movement; this final segment check is the hard
            // boundary guarantee. It also covers a click change while moving.
            companion.SetPosition(constrained);
            companion.Stop();
            ++myMovementCorrections;
        }
    }
    UpdateNavigationUI();
}

void GameWorld04_NavMesh::UpdateNavigationUI()
{
#ifndef _RETAIL
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos({viewport->WorkPos.x + 10.f, viewport->WorkPos.y + 10.f}, ImGuiCond_FirstUseEver);
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

    const char* assets[] = {"Course navmesh", "Course debug navmesh"};
    ImGui::Combo("Asset", &myNavMeshAsset, assets, 2);
    if (ImGui::Button("Reload selected FBX") && LoadNavigationMesh())
    {
        Actor& companion = myActorManager.GetActor(1);
        Vector2f position;
        int triangle = -1;
        if (myNavMesh.ClosestPoint(companion.GetPosition(), position, triangle))
        {
            companion.SetPosition(position);
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
        ImGui::SliderInt("Node (-1 = none)", &mySelectedNode, -1, static_cast<int>(myNavMesh.GetTriangles().size()) - 1);
        if (mySelectedNode >= 0)
        {
            const Vector2f centre = myNavMesh.GetTriangles()[mySelectedNode].centre;
            ImGui::Text("Centre: %.1f, %.1f", centre.x, centre.y);
            for (const CommonUtilities::PathfindingConnection& connection : myNavMesh.GetGraph()[mySelectedNode].myConnections)
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
    Tga::DebugDrawer& drawer = Tga::GraphicsEngine::GetInstance()->GetDebugDrawer();
    const std::vector<NavTriangle>& triangles = myNavMesh.GetTriangles();
    for (int index = 0; index < static_cast<int>(triangles.size()); ++index)
    {
        const NavTriangle& triangle = triangles[index];
        Tga::Color colour{0.35f, 0.5f, 0.65f, 1.f};
        const bool highlighted = index == myNavigationPath.startTriangle || index == myNavigationPath.goalTriangle || index == mySelectedNode;
        if (index == myNavigationPath.startTriangle) { colour = {0.3f, 0.8f, 1.f, 1.f}; }
        if (index == myNavigationPath.goalTriangle) { colour = {1.f, 0.4f, 0.85f, 1.f}; }
        if (index == mySelectedNode) { colour = {1.f, 1.f, 1.f, 1.f}; }
        if (myShowNavMesh || highlighted)
        {
            for (int edge = 0; edge < 3; ++edge)
            {
                drawer.DrawLine(triangle.vertices[edge].ToTga(), triangle.vertices[(edge + 1) % 3].ToTga(), colour);
            }
        }
        if (myShowNodes)
        {
            drawer.DrawCircle(triangle.centre.ToTga(), 3.f, colour);
        }
        if (myShowConnections || index == mySelectedNode)
        {
            for (const CommonUtilities::PathfindingConnection& connection : myNavMesh.GetGraph()[index].myConnections)
            {
                drawer.DrawLine(triangle.centre.ToTga(), triangles[connection.myNodeIndex].centre.ToTga(), colour);
            }
        }
    }
    if (myShowRawPath) { DrawPath(drawer, myNavigationPath.rawPoints, {1.f, 0.65f, 0.15f, 1.f}); }
    if (myShowSmoothPath) { DrawPath(drawer, myCompanionController->GetPath(), {0.25f, 1.f, 0.4f, 1.f}); }
    if (myShowPortals)
    {
        for (const NavPortal& portal : myNavigationPath.portals)
        {
            drawer.DrawLine(portal.left.ToTga(), portal.right.ToTga(), {0.85f, 0.85f, 1.f, 1.f});
            drawer.DrawCircle(portal.left.ToTga(), 6.f, {0.2f, 0.6f, 1.f, 1.f});
            drawer.DrawCircle(portal.right.ToTga(), 6.f, {1.f, 0.4f, 0.75f, 1.f});
        }
    }
    if (myShowTargets)
    {
        drawer.DrawCircle(myRequestedTarget.ToTga(), 12.f, {1.f, 0.8f, 0.2f, 1.f});
        if (myNavigationPath.goalTriangle >= 0)
        {
            drawer.DrawCircle(myNavigationPath.resolvedTarget.ToTga(), 8.f, {0.25f, 1.f, 0.4f, 1.f});
            drawer.DrawLine(myRequestedTarget.ToTga(), myNavigationPath.resolvedTarget.ToTga(), {0.7f, 0.7f, 0.7f, 1.f});
        }
    }
#endif
    myActorManager.Draw();
    for (std::size_t index = 0; index < 2; ++index)
    {
        const Actor& actor = myActorManager.GetActor(index);
        myLabels[index]->SetPosition((actor.GetPosition() + Vector2f{-30.f, 32.f}).ToTga());
        myLabels[index]->SetColor(actor.GetSpriteInstanceData().color);
        myLabels[index]->Render();
    }
}
