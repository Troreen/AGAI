#include "GameWorld.h"

#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/application.h>

#include "Actor.h"
#include "Boid.h"
#include "WanderController.h"
#include "BoidDebugRenderer.h"
#include "TraversalBounds.h"

#include <imgui/imgui.h>
#include <string>
#include <random>

using namespace Tga;

GameWorld::GameWorld()
{
}

GameWorld::~GameWorld()
{
}

void GameWorld::Init()
{
	Tga::Vector2ui intResolution = Tga::Application::GetInstance()->GetRenderSize();
	Tga::Vector2f resolution = { (float)intResolution.x, (float)intResolution.y };
	
	myTraversalBounds = std::make_shared<RectTraversalBounds>(resolution/4.f, resolution/(4.f/3.f));

	constexpr int boidCount = 1024;
	std::mt19937 randomGenerator(7u);
	std::uniform_real_distribution<float> randomX(80.f, resolution.x - 80.f);
	std::uniform_real_distribution<float> randomY(80.f, resolution.y - 80.f);

	for (int i = 0; i < boidCount; ++i)
	{
		WanderControllerData wanderData;
		wanderData.randomSeed = static_cast<unsigned int>(i + 1);
		wanderData.behaviorWeight = 0.35f;

		Boid& boid = myActorManager.CreateBoid(
			{ randomX(randomGenerator), randomY(randomGenerator) },
			"sprites/coolFish.png",
			std::make_unique<WanderController>(wanderData));
		boid.SetMaxSpeed(1000.f);
		boid.SetMaxForce(1000.f);
		boid.SetRadius(6.f);
		if (i == 0)
			boid.SetColor(Tga::Color(1, 0, 0, 1));
		boid.GetController()->SetTraversalBounds(myTraversalBounds);
	}
}

void GameWorld::Update(float aTimeDelta)
{
	myActorManager.Update(aTimeDelta);
}

void GameWorld::Render()
{
	auto editActorData = [](Actor& aActor, const char* aSuffix)
	{
		float maxSpeed = aActor.GetMaxSpeed();
		float maxForce = aActor.GetMaxForce();
		float mass = aActor.GetMass();
		float radius = aActor.GetRadius();
		if (ImGui::SliderFloat((std::string("Max Speed##") + aSuffix).c_str(), &maxSpeed, 0.f, 1000.f)) aActor.SetMaxSpeed(maxSpeed);
		if (ImGui::SliderFloat((std::string("Max Force##") + aSuffix).c_str(), &maxForce, 0.f, 2000.f)) aActor.SetMaxForce(maxForce);
		if (ImGui::SliderFloat((std::string("Mass##") + aSuffix).c_str(), &mass, 0.1f, 20.f)) aActor.SetMass(mass);
		if (ImGui::SliderFloat((std::string("Radius##") + aSuffix).c_str(), &radius, 0.f, 100.f)) aActor.SetRadius(radius);
	};

	if (ImGui::Begin("Boid Settings"))
	{
		if (!myActorManager.GetBoids().empty())
		{
			Boid& boid = *myActorManager.GetBoids().front();
			BoidSettings settings = boid.GetBoidSettings();
			editActorData(boid, "Boid");
			ImGui::SliderFloat("Perception Radius##Boid", &settings.perceptionRadius, 20.f, 500.f);
			ImGui::SliderFloat("Avoidance Radius##Boid", &settings.avoidanceRadius, 5.f, 200.f);
			ImGui::SliderFloat("Alignment##Boid", &settings.alignmentWeight, 0.f, 5.f);
			ImGui::SliderFloat("Cohesion##Boid", &settings.cohesionWeight, 0.f, 5.f);
			ImGui::SliderFloat("Separation##Boid", &settings.separationWeight, 0.f, 5.f);
			for (Boid* flockBoid : myActorManager.GetBoids())
				flockBoid->SetBoidSettings(settings);
		}
	}
	ImGui::End();

	myActorManager.Draw();

#if !IS_RETAIL_BUILD
	if (!myActorManager.GetBoids().empty())
	{
		BoidDebugRenderer debugRenderer;
		debugRenderer.DrawSelected(*myActorManager.GetBoids().front());
	}
#endif
}
