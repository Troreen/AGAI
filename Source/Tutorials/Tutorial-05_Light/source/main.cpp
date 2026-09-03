#include <string>

#include "../../TutorialCommon/TutorialCommon.h"
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/graphics/Camera.h>
#include <tge/sprite/sprite.h>
#include <tge/shaders/SpriteShader.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/Application.h>
#include <tge/settings/settings.h>

void Go( void );
int main( const int /*argc*/, const char * /*argc*/[] )
{
    Go();

    return 0;
}


void Go( void )
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
	{
		TutorialCommon::Init(L"TGE: Tutorial 5");

		Tga::Application& engine = *Tga::Application::GetInstance();
		Tga::SpriteDrawer& spriteDrawer(Tga::GraphicsEngine::GetInstance()->GetSpriteDrawer());

		Tga::SpriteSharedData spriteSharedData = {};

		// Set normal map
		Tga::Texture* normalMap = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("square_n.dds");
		spriteSharedData.maps[Tga::NORMAL_MAP] = normalMap;

		Tga::SpriteShader customShader; // Create
		customShader.Init("shaders/instanced_sprite_shader_vs", "shaders/LambertModelShaderPS");
		spriteSharedData.customShader = &customShader;

		// Create instance data. 
		Tga::Sprite2DInstanceData spriteInstance = {};

		// Setting the pivot so all operations will be in the middle of the image (rotation, position, etc.)
		spriteInstance.pivot = Tga::Vector2f(0.5f, 0.5f);
		spriteInstance.size = 0.05f;

		const int numberOfLogos = 500;
		std::vector<Tga::Vector2f> positions;
		for (int i = 0; i < numberOfLogos; i++)
		{
			float randX = (float)(rand() % 100) * 0.01f;
			float randY = (float)(rand() % 100) * 0.01f;
			positions.push_back({ randX , randY * engine.GetRenderSizeRatioInversed() });
		}

		Tga::AmbientLight ambientLight = {{0.f, 0.f, 0.f}};
		// LIGHT (Up to 8)

		srand((unsigned int)time(0));

		auto random0to1 = []() { return (rand() % 256 / 256.f); };

		std::vector<Tga::PointLight> myLights;
		for (int i = 0; i < NUMBER_OF_LIGHTS_ALLOWED; i++)
		{
			Tga::PointLight light(Tga::Vector3f(random0to1(), random0to1() * engine.GetRenderSizeRatioInversed(), -0.1f), 0.01f*Tga::Color(random0to1(), random0to1(), random0to1(), 1), 0.2f);
			myLights.push_back(light);
		}

		float timer = 0;
		while (true)
		{
			timer += 1.0f / 60.0f;
			if (!Tga::Application::GetInstance()->BeginFrame() || !Tga::GraphicsEngine::GetInstance()->BeginFrame())
			{
				break;
			}

			// Setting a custom camera, so the X coords go between 0 and 1:
			Tga::Camera camera;
			camera.SetOrtographicProjection(0.f, 1.f, 0.f, 1.f * engine.GetRenderSizeRatioInversed(), 0.f, 1.f);
			Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack().SetCamera(camera);

			Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack().SetAmbientLight(ambientLight);

			Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack().ClearPointLights();
			for (Tga::PointLight& light : myLights)
			{
				Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack().AddPointLight(light);
			}

			{
				Tga::SpriteBatchScope batch = spriteDrawer.BeginBatch(spriteSharedData);

				// Render all the loggos onto the sprite
				for (int i = 0; i < numberOfLogos; i++)
				{
					spriteInstance.position = { positions[i].x, positions[i].y };
					spriteInstance.rotation = static_cast<float>(timer * ((i%8)/8.f - 0.5f));

					spriteInstance.textureRect = { 0.f, 0.f, (float)(1 + (i % 3)), (float)(1 + (i % 3))};
					batch.Draw(spriteInstance);
				}
			}

			Tga::GraphicsEngine::GetInstance()->EndFrame();
			Tga::Application::GetInstance()->EndFrame();
		}
	}
	Tga::GraphicsEngine::GetInstance()->Shutdown();
	Tga::Application::GetInstance()->Shutdown();
}
