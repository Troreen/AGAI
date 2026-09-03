#pragma once

#include <tge/scene/SceneObjectDefinitionManager.h>
#include <tge/graphics/Camera.h>
#include <tge/shaders/ModelShader.h>
#include <tge/model/ModelInstance.h>

namespace Tga 
{
	class InputManager;
	class Scene;
}

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
	Tga::Camera myCamera;
	Tga::SceneObjectDefinitionManager mySceneObjectDefinitionManager;
	Tga::ModelShader myDropShadowShader;
	Tga::ModelInstance myShadowCubeInstance;
};