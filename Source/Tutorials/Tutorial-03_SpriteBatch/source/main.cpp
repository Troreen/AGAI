#include <string>
#include <tge/Application.h>
#include <tge/log/Log.h>

#include <tge/graphics/GraphicsEngine.h>
#include <tge/sprite/sprite.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include "../../TutorialCommon/TutorialCommon.h"
#include <tge/settings/settings.h>

#include <tge/graphics/GraphicsStateStack.h>

void Go( void );
int main( const int /*argc*/, const char * /*argc*/[] )
{
    Go();

    return 0;
}

// This is where the application starts of for real. By keeping this in it's own file
// we will have the same behaviour for both console and windows startup of the
// application.
//
void Go( void )
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
	{
		TutorialCommon::Init(L"TGE: Tutorial 3");

		Tga::Application& engine = *Tga::Application::GetInstance();
		Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

		Tga::Vector2ui intResolution = engine.GetRenderSize();
		Tga::Vector2f resolution = { (float)intResolution.x, (float)intResolution.y};

		// Get a sprite drawer, for drawing sprites
		Tga::SpriteDrawer& spriteDrawer(graphicsEngine.GetSpriteDrawer());

		Tga::SpriteSharedData sharedData;
		sharedData.texture = graphicsEngine.GetTextureManager().GetTexture("sprites/tge_logo_w.dds");

		std::vector<Tga::Sprite2DInstanceData> instances;
		for (unsigned int i = 0; i < 100000; i++)
		{
			float randomX = static_cast<float>(rand() % 1000) / 1000.0f;
			float randomY = static_cast<float>(rand() % 1000) / 1000.0f;

			Tga::Sprite2DInstanceData instance = {};

			// Random position on screen
			instance.position = resolution*Tga::Vector2f(randomX, randomY);
			// Color based on its location on screen
			instance.color = Tga::Color(randomX, randomY, randomX, 1);
			// Size is 10% of screen
			instance.size = Tga::Vector2f(0.1f, 0.1f) * resolution.y;
			instances.push_back(instance);
		}

		// MAIN LOOP
		while (true)
		{
			if (!engine.BeginFrame() || !graphicsEngine.BeginFrame())
			{
				break;
			}

			//Tga::GraphicsEngine::GetInstance().GetGraphicsStateStack().SetBlendState(Tga::BlendState::Disabled);

			spriteDrawer.Draw(sharedData, &instances[0], instances.size());

			graphicsEngine.EndFrame();
			engine.EndFrame();
		}
	}

	Tga::GraphicsEngine::Shutdown();
	Tga::Application::Shutdown();
}
