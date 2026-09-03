#pragma once

#include <tge/math/Vector3.h>
#include <tge/math/Matrix4x4.h>
#include <tge/model/ModelInstance.h>
#include <tge/graphics/DepthBuffer.h>
#include <tge/graphics/RenderTarget.h>
#include <tge/graphics/FullscreenEffect.h>
#include <tge/shaders/ModelShader.h>
#include <tge/scene/SceneObjectDefinitionManager.h>
#include <tge/graphics/Camera.h>
#include <tge/text/Text.h>

#include <vector>

namespace Tga 
{
	class InputManager;
	class Scene;
}

struct InstanceData
{
	Tga::Matrix4x4f transform;
	Tga::ModelInstance instance;
};

class GameWorld
{
public:
	GameWorld(); 
	~GameWorld();

	void Init();
	void LoadScene(Tga::Scene& aScene);
	void Update(float aTimeDelta, Tga::InputManager& inputManager);
	void Render();

private:
	void DrawOpaqueObjects(bool aIsZPrepass = false);

	Tga::Camera myCamera;
	Tga::SceneObjectDefinitionManager mySceneObjectDefinitionManager;

	Tga::Vector3f myPlayerPosition;
	Tga::Vector3f myDirectionalLightRotation;

	Tga::ModelInstance myPlayerInstance;
	bool myHasPlayerInstance = false;

	Tga::DepthBuffer myDirectionalShadowMap;
	Tga::DepthBuffer myZPrepassDepthBuffer;
	Tga::RenderTarget mySsaoRenderTarget;

	Tga::FullscreenEffect mySsaoEffect;
	Tga::ModelShader myLambertShadowSsaoShader;

	std::vector<InstanceData> mySceneInstances;

	Tga::Text myExplanationText;
	bool myUseShadowsAndAO = true;
};


