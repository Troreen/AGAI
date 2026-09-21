#include "GameWorld.h"

#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/application.h>

#include "Actor.h"
#include "Controllers/WanderController.h"
#include "Controllers/AlignmentController.h"
#include "Controllers/CohesionController.h"
#include "Controllers/SeparationController.h"
#include "Controllers/ObstacleAvoidanceController.h"
#include "Interfaces/TraversalBounds.h"

#include <imgui/imgui.h>
#include <string>
#include <random>
#include <array>
#include <algorithm>
#include <stdexcept>

GameWorld::GameWorld()
{
}

GameWorld::~GameWorld()
{
}

void GameWorld::Init()
{
    const Tga::Vector2ui intResolution = Tga::Application::GetInstance()->GetRenderSize();
    const CommonUtilities::Vector2f resolution = {
        static_cast<float>(intResolution.x), static_cast<float>(intResolution.y) };

    // Use 90% of each screen dimension, leaving a 5% margin on every side.
    myTraversalBounds = std::make_shared<RectTraversalBounds>(resolution * 0.05f, resolution * 0.95f);
    // Small circular test obstacles. Positions and radii use the same units as the boid.
    const float obstacleRadius = resolution.y * 0.045f;
    myObstacles = {
        { { resolution.x * 0.1f, resolution.y * 0.5f }, obstacleRadius },
        { { resolution.x * 0.6f, resolution.y * 0.4f }, obstacleRadius },
        { { resolution.x * 0.6f, resolution.y * 0.65f }, obstacleRadius }
    };
    const float cellSize = myFlockingSettings.perceptionRadius;
    const int gridWidth = static_cast<int>(std::ceil(resolution.x / cellSize));
    const int gridHeight = static_cast<int>(std::ceil(resolution.y / cellSize));
    myFlockingGrid.Init({}, cellSize, gridWidth, gridHeight);

    std::mt19937 randomGenerator(7u);
    // Start boid inside the walls with the same body clearance used by avoidance casts.
    constexpr float boidRadius = 6.f;
    const float wallClearance = boidRadius + myObstacleSettings.clearance;
    std::uniform_real_distribution<float> randomX(
        myTraversalBounds->GetMin().x + wallClearance, myTraversalBounds->GetMax().x - wallClearance);
    std::uniform_real_distribution<float> randomY(
        myTraversalBounds->GetMin().y + wallClearance, myTraversalBounds->GetMax().y - wallClearance);

    for (std::size_t i = 0; i < actorCount; ++i)
    {
        WanderControllerData wanderData;
        wanderData.randomSeed = static_cast<unsigned int>(i + 1);
        wanderData.behaviorWeight = 0.35f;

        CommonUtilities::Vector2f spawnPosition;
        bool overlapsObstacle;
        do
        {
            spawnPosition = { randomX(randomGenerator), randomY(randomGenerator) };
            overlapsObstacle = false;
            for (const auto& obstacle : myObstacles)
            {
                const float spawnClearance = obstacle.radius + boidRadius + myObstacleSettings.clearance;
                if (spawnPosition.DistanceSqr(obstacle.position) < spawnClearance * spawnClearance)
                    overlapsObstacle = true;
            }
        } while (overlapsObstacle);

        Actor& actor = myActorManager.CreateActor(
            spawnPosition,
            "sprites/CoolFish.png",
            std::make_unique<WanderController>(wanderData));
        actor.SetMaxSpeed(2500.f);
        actor.SetMaxForce(1000.f);
        actor.SetRadius(boidRadius);
        if (i == 0)
        {
            actor.SetColor(Tga::Color(1, 0, 0, 1));
        }
        Controller* const wanderController = actor.GetController();
        wanderController->SetTraversalBounds(myTraversalBounds);
        actor.AddController(std::make_unique<AlignmentController>(myFlockingSettings));
        actor.AddController(std::make_unique<CohesionController>(myFlockingSettings));
        actor.AddController(std::make_unique<SeparationController>(myFlockingSettings));
        actor.AddController(std::make_unique<ObstacleAvoidanceController>(myObstacleSettings, myObstacles, myTraversalBounds));
    }
}

void GameWorld::Update(float aTimeDelta)
{
    UpdateDebugUI();
    RebuildFlockingGrid();
    // Allocate once on the stack and reuse it for every actor, without heap allocations.
    std::array<const Actor*, actorCount> storage;
    for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
    {
        Actor& actor = myActorManager.GetActor(index);
        const auto neighbours = actor.NeedsNeighbours()
            ? FindNeighbours(actor, myFlockingSettings.perceptionRadius, storage)
            : std::span<const Actor* const>{};
        actor.CalculateSteering(aTimeDelta, neighbours);
    }
    // Every query sees the same frame: finish steering before moving any actors.
    for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
        myActorManager.GetActor(index).UpdateMovement(aTimeDelta);
}

void GameWorld::RebuildFlockingGrid()
{
    myFlockingGrid.Clear();
    for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
        myFlockingGrid.InsertCircle(static_cast<int>(index), myActorManager.GetActor(index).GetPosition(), 0.f);
}

std::span<const Actor* const> GameWorld::FindNeighbours(
    const Actor& aActor, float aRadius, std::span<const Actor*> aStorage) const
{
    std::size_t count = 0;
    const float radiusSqr = aRadius * aRadius;
    myFlockingGrid.VisitCellsOverlappingCircle(aActor.GetPosition(), aRadius,
        [&](int aCellIndex)
        {
            for (const int index : myFlockingGrid.GetObjectsInCell(aCellIndex))
            {
                const Actor& candidate = myActorManager.GetActor(static_cast<std::size_t>(index));
                if (&candidate == &aActor || aActor.GetPosition().DistanceSqr(candidate.GetPosition()) > radiusSqr)
                    continue;

                // Fail explicitly if the population ever outgrows the caller's buffer; never drop neighbours.
                if (count == aStorage.size())
                    throw std::length_error("Neighbour scratch buffer is too small");
                aStorage[count++] = &candidate;
            }
            return true;
        });
    return { aStorage.data(), count };
}

void GameWorld::UpdateDebugUI()
{
    if (!ImGui::Begin("Flocking Settings"))
    {
        ImGui::End();
        return;
    }

    if (myActorManager.GetActorCount() > 0)
    {
        Actor& actor = myActorManager.GetActor(0);
        float maxSpeed = actor.GetMaxSpeed();
        float maxForce = actor.GetMaxForce();
        float mass = actor.GetMass();
        float radius = actor.GetRadius();

        const bool maxSpeedChanged = ImGui::SliderFloat("Max Speed", &maxSpeed, 0.f, 1000.f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Maximum travel speed. Higher values let boid move faster.");
        if (maxSpeedChanged)
        {
            for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
            {
                myActorManager.GetActor(index).SetMaxSpeed(maxSpeed);
            }
        }
        const bool maxForceChanged = ImGui::SliderFloat("Max Force", &maxForce, 0.f, 2000.f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Maximum steering force. Higher values allow sharper turns and faster acceleration.");
        if (maxForceChanged)
        {
            for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
            {
                myActorManager.GetActor(index).SetMaxForce(maxForce);
            }
        }
        const bool massChanged = ImGui::SliderFloat("Mass", &mass, 0.1f, 20.f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Resistance to acceleration. Higher mass makes turning and speeding up slower.");
        if (massChanged)
        {
            for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
            {
                myActorManager.GetActor(index).SetMass(mass);
            }
        }
        const bool radiusChanged = ImGui::SliderFloat("Radius", &radius, 0.f, 100.f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("boid radius used by obstacle casts and optional containment; does not change sprite size.");
        if (radiusChanged)
        {
            for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
            {
                myActorManager.GetActor(index).SetRadius(radius);
            }
        }
    }
    ImGui::SliderFloat("Perception Radius", &myFlockingSettings.perceptionRadius, 20.f, 500.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("How far boid can see. Only boid within this distance influence flocking.");
    ImGui::SliderFloat("Avoidance Radius", &myFlockingSettings.avoidanceRadius, 5.f, 200.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Push away from detected neighbours inside this distance. Limited by perception radius.");
    ImGui::SliderFloat("Alignment", &myFlockingSettings.alignmentWeight, 0.f, 5.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Match the average neighbour velocity. Zero disables this force.");
    ImGui::SliderFloat("Cohesion", &myFlockingSettings.cohesionWeight, 0.f, 5.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Move towards the average neighbour position. Zero disables this force.");
    ImGui::SliderFloat("Separation", &myFlockingSettings.separationWeight, 0.f, 5.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Push away from close neighbours. Zero disables this force.");
    ImGui::Separator();
    ImGui::TextUnformatted("Obstacle avoidance");
    ImGui::SliderFloat("Minimum ray length", &myObstacleSettings.minimumRayLength, 10.f, 500.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Minimum distance checked for obstacles, even while moving slowly.");
    ImGui::SliderFloat("Look ahead (seconds)", &myObstacleSettings.lookAheadSeconds, 0.f, 3.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Check this many seconds of travel ahead. Longer rays give fast boid more time to turn.");
    ImGui::SliderFloat("Obstacle clearance", &myObstacleSettings.clearance, 0.f, 100.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Extra gap between the boid's collision radius and an obstacle.");
    ImGui::SliderFloat("Obstacle avoidance strength", &myObstacleSettings.weight, 0.f, 30.f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("How strongly avoidance competes with flocking. Zero turns obstacle avoidance off.");
    if (ImGui::Checkbox("Enable old containment (opt in)", &myUseContainment))
    {
        for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
        {
            Controller* wander = myActorManager.GetActor(index).GetController();
            auto data = wander->GetControllerData();
            data.useContainment = myUseContainment;
            wander->SetControllerData(data);
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Add the old containment steering as well as ray-based wall avoidance. Off by default.");
    ImGui::Checkbox("Show traversal bounds (purple)", &myShowTraversalBounds);
    ImGui::Checkbox("Show obstacles (orange)", &myShowObstacles);
    ImGui::Checkbox("Show obstacle avoidance rays", &myShowAvoidanceRays);
    ImGui::Checkbox("Show rays for all boid", &myShowAllAvoidanceRays);
    ImGui::TextUnformatted("Rays: red = blocked, green = clear; cyan arrow = chosen turn.");
    ImGui::TextUnformatted("Rays use current positions; all blocked means brake.");
    ImGui::Separator();
    ImGui::TextUnformatted("Debug: red boid and its neighbours (current positions)");
    ImGui::Checkbox("Detection radius (yellow)", &myShowDetectionRadius);
    ImGui::Checkbox("Cohesion centre and lines (green)", &myShowCohesion);
    ImGui::Checkbox("Neighbour velocities and average (cyan)", &myShowAlignment);
    ImGui::Checkbox("Separation push directions (red)", &myShowSeparation);
    ImGui::TextUnformatted("Velocity lines show 0.1 seconds of travel.");
    ImGui::TextUnformatted("Red lines grow longer as neighbours get closer.");
    ImGui::End();
}

void GameWorld::Render()
{
    myActorManager.Draw();
    DrawFlockingDebug();
    DrawObstacleDebug();
}

void GameWorld::DrawObstacleDebug()
{
#ifndef _RETAIL
    auto& drawer = Tga::GraphicsEngine::GetInstance()->GetDebugDrawer();
    if (myShowTraversalBounds && myTraversalBounds)
    {
        const auto min = myTraversalBounds->GetMin();
        const auto max = myTraversalBounds->GetMax();
        const Tga::Color purple(0.8f, 0.3f, 1.f, 1.f);
        drawer.DrawLine(min.ToTga(), { max.x, min.y }, purple);
        drawer.DrawLine({ max.x, min.y }, max.ToTga(), purple);
        drawer.DrawLine(max.ToTga(), { min.x, max.y }, purple);
        drawer.DrawLine({ min.x, max.y }, min.ToTga(), purple);
    }
    if (myShowObstacles)
    {
        for (const auto& obstacle : myObstacles)
            drawer.DrawCircle(obstacle.position.ToTga(), obstacle.radius, Tga::Color(1.f, 0.55f, 0.1f, 1.f));
    }
    if (!myShowAvoidanceRays || myObstacleSettings.weight <= 0.f)
        return;

    const std::size_t count = myShowAllAvoidanceRays ? myActorManager.GetActorCount()
        : (std::min)(myActorManager.GetActorCount(), std::size_t{ 1 });
    for (std::size_t index = 0; index < count; ++index)
    {
        const Actor& actor = myActorManager.GetActor(index);
        for (const auto& controller : actor.GetControllers())
        {
            const auto* avoidance = dynamic_cast<const ObstacleAvoidanceController*>(controller.get());
            if (!avoidance)
                continue;
            const auto info = avoidance->EvaluateAvoidance(actor);
            for (std::size_t rayIndex = 0; rayIndex < info.testedRayCount; ++rayIndex)
            {
                const auto& ray = info.rays[rayIndex];
                const auto end = actor.GetPosition() + ray.direction * ray.distance;
                const Tga::Color color = ray.blocked ? Tga::Color(1.f, 0.2f, 0.2f, 1.f)
                    : Tga::Color(0.2f, 1.f, 0.3f, 1.f);
                drawer.DrawLine(actor.GetPosition().ToTga(), end.ToTga(), color);
            }
            if (info.forwardBlocked && info.foundClearDirection)
                drawer.DrawArrow(actor.GetPosition().ToTga(),
                    (actor.GetPosition() + info.direction * 50.f).ToTga(), Tga::Color(0.f, 1.f, 1.f, 1.f), 6.f);
        }
    }
#endif
}

void GameWorld::DrawFlockingDebug()
{
#ifndef _RETAIL
    if (myActorManager.GetActorCount() == 0 ||
        !(myShowDetectionRadius || myShowCohesion || myShowAlignment || myShowSeparation))
        return;

    // Movement has finished, so refresh the grid to match the positions being drawn.
    RebuildFlockingGrid();
    std::array<const Actor*, actorCount> storage;
    const Actor& actor = myActorManager.GetActor(0);
    const auto neighbours = FindNeighbours(actor, myFlockingSettings.perceptionRadius, storage);
    const auto position = actor.GetPosition();
    auto& drawer = Tga::GraphicsEngine::GetInstance()->GetDebugDrawer();
    const Tga::Color yellow(1.f, 0.85f, 0.f, 1.f);
    const Tga::Color green(0.2f, 1.f, 0.3f, 1.f);
    const Tga::Color cyan(0.f, 0.8f, 1.f, 1.f);
    const Tga::Color red(1.f, 0.15f, 0.15f, 1.f);
    // Draw velocities as this many seconds of travel, keeping fast boid's lines readable.
    constexpr float velocitySeconds = 0.1f;
    // Maximum length of each red push line; closer neighbours produce longer lines.
    constexpr float pushLineLength = 25.f;

    if (myShowDetectionRadius)
        drawer.DrawCircle(position.ToTga(), myFlockingSettings.perceptionRadius, yellow);
    if (neighbours.empty())
        return;

    CommonUtilities::Vector2f centre = {};
    CommonUtilities::Vector2f averageVelocity = {};
    for (const Actor* neighbour : neighbours)
    {
        centre += neighbour->GetPosition();
        averageVelocity += neighbour->GetVelocity();
    }
    centre /= static_cast<float>(neighbours.size());
    averageVelocity /= static_cast<float>(neighbours.size());

    if (myShowCohesion)
    {
        drawer.DrawCircle(centre.ToTga(), 5.f, green);
        drawer.DrawArrow(position.ToTga(), centre.ToTga(), green, 6.f);
    }
    if (myShowAlignment)
        drawer.DrawArrow(position.ToTga(), (position + averageVelocity * velocitySeconds).ToTga(), cyan, 6.f);

    for (const Actor* neighbour : neighbours)
    {
        const auto neighbourPosition = neighbour->GetPosition();
        if (myShowCohesion)
            drawer.DrawLine(neighbourPosition.ToTga(), centre.ToTga(), green);
        if (myShowAlignment)
            drawer.DrawLine(neighbourPosition.ToTga(),
                (neighbourPosition + neighbour->GetVelocity() * velocitySeconds).ToTga(), cyan);
        if (myShowSeparation)
        {
            const auto offset = position - neighbourPosition;
            const float distanceSqr = offset.LengthSqr();
            const float radius = myFlockingSettings.avoidanceRadius;
            // Match separation's distance weighting, including its coincident-position guard.
            if (distanceSqr <= 0.0001f || distanceSqr >= radius * radius || radius <= 0.f)
                continue;
            const float closeness = (radius - offset.Length()) / radius;
            const auto push = offset.GetNormalized() * (closeness * pushLineLength);
            drawer.DrawLine(position.ToTga(), (position + push).ToTga(), red);
            drawer.DrawLine(neighbourPosition.ToTga(), (neighbourPosition - push).ToTga(), red);
        }
    }
#endif
}
