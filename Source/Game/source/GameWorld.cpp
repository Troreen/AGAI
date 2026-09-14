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
#include "Interfaces/TraversalBounds.h"

#include <imgui/imgui.h>
#include <string>
#include <random>

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
	
	myTraversalBounds = std::make_shared<RectTraversalBounds>(resolution/4.f, resolution/(4.f/3.f));
	const float cellSize = myFlockingSettings.perceptionRadius;
	const int gridWidth = static_cast<int>(std::ceil(resolution.x / cellSize));
	const int gridHeight = static_cast<int>(std::ceil(resolution.y / cellSize));
	myFlockingGrid.Init({}, cellSize, gridWidth, gridHeight);

	constexpr int actorCount = 2048;
	std::mt19937 randomGenerator(7u);
	std::uniform_real_distribution<float> randomX(80.f, resolution.x - 80.f);
	std::uniform_real_distribution<float> randomY(80.f, resolution.y - 80.f);

	for (int i = 0; i < actorCount; ++i)
	{
		WanderControllerData wanderData;
		wanderData.randomSeed = static_cast<unsigned int>(i + 1);
		wanderData.behaviorWeight = 0.35f;

		Actor& actor = myActorManager.CreateActor(
			{ randomX(randomGenerator), randomY(randomGenerator) },
			"sprites/CoolFish.png",
			std::make_unique<WanderController>(wanderData));
		actor.SetMaxSpeed(1000.f);
		actor.SetMaxForce(1000.f);
		actor.SetRadius(6.f);
		if (i == 0)
		{
			actor.SetColor(Tga::Color(1, 0, 0, 1));
		}
		Controller* const wanderController = actor.GetController();
		wanderController->SetTraversalBounds(myTraversalBounds);
		actor.AddController(std::make_unique<AlignmentController>(myFlockingSettings));
		actor.AddController(std::make_unique<CohesionController>(myFlockingSettings));
		actor.AddController(std::make_unique<SeparationController>(myFlockingSettings));
	}
}

void GameWorld::Update(float aTimeDelta)
{
	UpdateDebugUI();
	myActorManager.BuildNeighbourLists(myFlockingGrid, myFlockingSettings.perceptionRadius);
	myActorManager.Update(aTimeDelta);
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
		if (maxSpeedChanged)
		{
			for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
			{
				myActorManager.GetActor(index).SetMaxSpeed(maxSpeed);
			}
		}
		const bool maxForceChanged = ImGui::SliderFloat("Max Force", &maxForce, 0.f, 2000.f);
		if (maxForceChanged)
		{
			for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
			{
				myActorManager.GetActor(index).SetMaxForce(maxForce);
			}
		}
		const bool massChanged = ImGui::SliderFloat("Mass", &mass, 0.1f, 20.f);
		if (massChanged)
		{
			for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
			{
				myActorManager.GetActor(index).SetMass(mass);
			}
		}
		const bool radiusChanged = ImGui::SliderFloat("Radius", &radius, 0.f, 100.f);
		if (radiusChanged)
		{
			for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
			{
				myActorManager.GetActor(index).SetRadius(radius);
			}
		}
	}
	ImGui::SliderFloat("Perception Radius", &myFlockingSettings.perceptionRadius, 20.f, 500.f);
	ImGui::SliderFloat("Avoidance Radius", &myFlockingSettings.avoidanceRadius, 5.f, 200.f);
	ImGui::SliderFloat("Alignment", &myFlockingSettings.alignmentWeight, 0.f, 5.f);
	ImGui::SliderFloat("Cohesion", &myFlockingSettings.cohesionWeight, 0.f, 5.f);
	ImGui::SliderFloat("Separation", &myFlockingSettings.separationWeight, 0.f, 5.f);
	ImGui::End();
}

void GameWorld::Render()
{
	myActorManager.Draw();
}
