#include "GameWorld.h"

#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/application.h>

#include "Actor.h"
#include "ArriveController.h"
#include "SeekerController.h"
#include "WanderController.h"
#include "SteeringDebugRenderer.h"
#include "TraversalBounds.h"

#include <imgui/imgui.h>
#include <string>

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
	
	myTraversalBounds = std::make_shared<RectTraversalBounds>(Tga::Vector2f{}, resolution);

	TargetControllerData seekerData;
	seekerData.randomSeed = 1u;
	ArriveControllerData arriveData;
	arriveData.randomSeed = 2u;
	WanderControllerData wanderData;
	wanderData.randomSeed = 3u;
	Actor& seeker = myActorManager.CreateActor(resolution * 0.5f, "sprites/robot1.png", std::make_unique<SeekerController>(seekerData));
	Actor& arriver = myActorManager.CreateActor(resolution * 0.5f, "sprites/robot2.png", std::make_unique<ArriveController>(arriveData));
	Actor& wanderer = myActorManager.CreateActor(resolution * 0.5f, "sprites/human.png", std::make_unique<WanderController>(wanderData));

	for (Actor* actor : { &seeker, &arriver, &wanderer })
		actor->GetController()->SetTraversalBounds(myTraversalBounds);
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

	auto editCommonData = [](ControllerData& aData, const char* aSuffix)
	{
		ImGui::Checkbox((std::string("Use Containment##") + aSuffix).c_str(), &aData.useContainment);
		ImGui::SliderFloat((std::string("Containment Weight##") + aSuffix).c_str(), &aData.containmentWeight, 0.f, 10.f);
		ImGui::SliderFloat((std::string("Boundary Look Ahead##") + aSuffix).c_str(), &aData.boundaryLookAhead, 0.f, 3.f);
		ImGui::SliderFloat((std::string("Boundary Clearance##") + aSuffix).c_str(), &aData.boundaryClearance, 0.f, 300.f);
	};

	if (ImGui::Begin("Controller Settings"))
	{
		if (ImGui::CollapsingHeader("Seeker", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Actor& seeker = myActorManager.GetActor(0);
			auto* controller = static_cast<SeekerController*>(seeker.GetController());
			TargetControllerData data = controller->GetTargetControllerData();
			editActorData(seeker, "Seeker");
			editCommonData(data, "Seeker");
			ImGui::SliderFloat("Target Reached Radius##Seeker", &data.targetReachedRadius, 0.f, 100.f);
			controller->SetTargetControllerData(data);
		}

		if (ImGui::CollapsingHeader("Arrive", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Actor& arriver = myActorManager.GetActor(1);
			auto* controller = static_cast<ArriveController*>(arriver.GetController());
			ArriveControllerData data = controller->GetArriveControllerData();
			editActorData(arriver, "Arrive");
			editCommonData(data, "Arrive");
			ImGui::SliderFloat("Slowing Radius##Arrive", &data.slowingRadius, 1.f, 500.f);
			ImGui::SliderFloat("Target Change Interval##Arrive", &data.targetChangeInterval, 0.01f, 20.f);
			controller->SetArriveControllerData(data);
		}

		if (ImGui::CollapsingHeader("Wander", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Actor& wanderer = myActorManager.GetActor(2);
			WanderController* wanderController = static_cast<WanderController*>(wanderer.GetController());
			WanderControllerData data = wanderController->GetWanderControllerData();
			editActorData(wanderer, "Wander");
			editCommonData(data, "Wander");
			ImGui::SliderFloat("Circle Radius##Wander", &data.circleRadius, 1.f, 300.f);
			ImGui::SliderFloat("Circle Distance##Wander", &data.circleDistance, 1.f, 500.f);
			ImGui::SliderFloat("Max Turn Rate##Wander", &data.maxTurnRate, 0.f, 10.f);
			ImGui::SliderFloat("Direction Change Interval##Wander", &data.directionChangeInterval, 0.01f, 2.f);
			wanderController->SetWanderControllerData(data);
		}
	}
	ImGui::End();

	myActorManager.Draw();

#if !IS_RETAIL_BUILD
	SteeringDebugRenderer debugRenderer;
	for (std::size_t i = 0; i < myActorManager.GetActorCount(); ++i)
	{
		const Actor& actor = myActorManager.GetActor(i);
		if (actor.GetController())
			debugRenderer.Draw(actor, *actor.GetController());
	}
#endif
}
