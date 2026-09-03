#include <string>
#include <tge/Application.h>
#include <tge/log/Log.h>

#include <tge/graphics/GraphicsEngine.h>
#include <tge/sprite/sprite.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include "../../TutorialCommon/TutorialCommon.h"
#include <tge/settings/settings.h>

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

	TutorialCommon::Init(L"TGE: Tutorial 2");
   
	Tga::Application& engine = *Tga::Application::GetInstance();
	Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

	// Get a sprite drawer, for drawing sprites
	Tga::SpriteDrawer& spriteDrawer(Tga::GraphicsEngine::GetInstance()->GetSpriteDrawer());

	// Sprite data is split into two parts. Shared data that is shared across multiple sprites and instance data that is unique per instance.
	// This split is done for performance reasons.

	// Create shared data and assign a texture.
	Tga::SpriteSharedData sharedData = {};
	sharedData.texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("sprites/tge_logo_b.dds");
	
	// Create instance data. 
	Tga::Sprite2DInstanceData spriteInstance = {};

	// Setting the pivot so all operations will be in the middle of the image (rotation, position, etc.)
	spriteInstance.pivot = Tga::Vector2f(0.5f, 0.5f);

	// Setting the size in pixels
	spriteInstance.size = { 300.f, 300.f };

	Tga::Vector2ui resolution = Tga::Application::GetInstance()->GetRenderSize();
	Tga::Vector2f center = { (float)resolution.x * 0.5f, (float)resolution.y * 0.5f };
	spriteInstance.position = center + Tga::Vector2f{300.f, 300.f};

	float timer = 0;
	while (true)
	{
		timer += 1.0f / 60.0f;
		if (!engine.BeginFrame() || !graphicsEngine.BeginFrame())
		{
			break;
		}

		{
			// Start a sprite batch. This allows multiple sprites to be drawn efficiently as long as they share the same shared data.
			// The batch will be completed when the batch scope goes out of scope
			Tga::SpriteBatchScope batch = spriteDrawer.BeginBatch(sharedData);

			// Set a new position
			spriteInstance.position = center + 300.f * Tga::Vector2f(cosf(timer), sinf(timer));
			// Set the rotation
			spriteInstance.rotation = cosf(timer);

			// Draw the sprite
			batch.Draw(spriteInstance);

			// using the same instance we reuse the image and set a new position
			spriteInstance.position = center + 300.f * Tga::Vector2f(sinf(timer), cosf(timer));
			spriteInstance.rotation = 0.f;

			// Draw a second time in a new position
			batch.Draw(spriteInstance);

			// Batch goes out of scope and flushes all rendering
		}
		graphicsEngine.EndFrame();
		engine.EndFrame();
	}
	Tga::GraphicsEngine::Shutdown();
	Tga::Application::Shutdown();
}
