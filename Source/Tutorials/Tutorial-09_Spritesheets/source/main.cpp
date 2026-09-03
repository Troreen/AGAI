#include <string>

#include <tge/drawers/DebugDrawer.h>
#include <tge/math/vector2.h>
#include <tge/Application.h>
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


void Go( void )
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
	{
		TutorialCommon::Init(L"TGE: Tutorial 9");

		// Get a sprite drawer, for drawing sprites
		Tga::SpriteDrawer& spriteDrawer(Tga::GraphicsEngine::GetInstance()->GetSpriteDrawer());

		// Sprite data is split into two parts. Shared data that is shared across multiple sprites and instance data that is unique per instance.
		// This split is done for performance reasons.

		// Create shared data and assign a texture.
		Tga::SpriteSharedData sharedData = {};
		Tga::Texture* texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("animation.dds");
		sharedData.texture = texture;

		Tga::Vector2ui intResolution = Tga::Application::GetInstance()->GetRenderSize();
		Tga::Vector2f resolution = { (float)intResolution.x, (float)intResolution.y };


		struct UV
		{
			UV(Tga::Vector2f aStart, Tga::Vector2f aEnd) { start = aStart; end = aEnd; }
			Tga::Vector2f start;
			Tga::Vector2f end;
		};

		const float addingUVX = 1.0f / 8.0f; // 8 sprites per row
		const float addingUVY = 1.0f / 8.0f; // 8 sprites per col
		std::vector<UV> myUvs;
		for (int j = 0; j < 8; j++)
		{
			for (int i = 0; i < 8; i++)
			{
				myUvs.push_back(UV({ addingUVX * i, addingUVY * j }, { (addingUVX * i) + addingUVX, (addingUVY * j) + addingUVY }));
			}
		}

		const float deltaTime = 1.0f / 60.0f;
		float timer = 0.0f;
		unsigned short aIndex = 0;
		while (true)
		{
			timer += deltaTime;
			if (!Tga::Application::GetInstance()->BeginFrame() || !Tga::GraphicsEngine::GetInstance()->BeginFrame())
			{
				break;
			}

			// Cycle the sheet
			if (timer >= 0.05f)
			{
				aIndex++;
				if (aIndex > 28)
				{
					aIndex = 0;
				}
				timer = 0.0f;
			}

			Tga::Sprite2DInstanceData instance = {};
			instance.size = texture->myImageSize / 8.f; // Setting size to an eigth of scale to get pixel size to match screen
			instance.position = resolution * 0.5f;
			instance.pivot = { 0.5f, 0.5f };
			instance.textureRect = { myUvs[aIndex].start.x, myUvs[aIndex].start.y, myUvs[aIndex].end.x, myUvs[aIndex].end.y };
			spriteDrawer.Draw(sharedData, instance);

			Tga::GraphicsEngine::GetInstance()->EndFrame();
			Tga::Application::GetInstance()->EndFrame();
		}
	}
	Tga::Application::Shutdown();
	Tga::GraphicsEngine::Shutdown();
}
