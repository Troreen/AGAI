#include <string>

#include "../../TutorialCommon/TutorialCommon.h"
#include <tge/drawers/CustomShapeDrawer.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/sprite/sprite.h>
#include <tge/Application.h>
#include <tge/graphics/Camera.h>
#include <tge/graphics/DX11.h>
#include <tge/primitives/CustomShape.h>
#include <tge/shaders/SpriteShader.h>
#include <tge/texture/TextureManager.h>
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
		TutorialCommon::Init(L"TGE: Tutorial 7");

		// Background, not needed, but beautiful
		Tga::CustomShape2D myShape;
		myShape.Reset();
		myShape.AddPoint({ 0.0f, 0.0f }, Tga::Color(1, 0, 0, 1));
		myShape.AddPoint({ 1.0f, 0.0f }, Tga::Color(0, 1, 0, 1));
		myShape.AddPoint({ 0.0f, 1.0f }, Tga::Color(0, 0, 1, 1));

		myShape.AddPoint({ 1.0f, 0.0f }, Tga::Color(0, 1, 0, 1));
		myShape.AddPoint({ 1.0f, 1.0f }, Tga::Color(0, 0, 1, 1));
		myShape.AddPoint({ 0.0f, 1.0f }, Tga::Color(0, 0, 1, 1));
		myShape.BuildShape();

		//The target texture we will render to instead of the screen, this is a sprite, which means it have all the nice features of one (rotation, position etc.)
		Tga::RenderTarget myRenderTargetTexture = Tga::RenderTarget::Create({1024,1024});
		Tga::RenderTarget& backBuffer = *Tga::DX11::BackBuffer;

		// Ordinary sprite that we will render onto the target
		Tga::SpriteSharedData logoSpriteSharedData = {};
		Tga::Texture* texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Sprites/tge_logo_b.dds");
		logoSpriteSharedData.texture = texture;
		Tga::Vector2f logoSize = texture->mySize;

		// We need a couple of tga loggos!
		const int numberOfLoggos = 20;
		std::vector<Tga::Sprite2DInstanceData> logoInstances;
		for (int i = 0; i < numberOfLoggos; i++)
		{
			float randX = (float)(rand() % 100) * 0.01f;
			float randY = (float)(rand() % 100) * 0.01f;
			Tga::Sprite2DInstanceData logo;
			logo.position = { randX , randY };
			logo.pivot = { 0.5f, 0.5f };
			logo.size = { 0.5f * logoSize.myX, 0.5f * logoSize.myY };

			logoInstances.push_back(logo);
		}

		// Create a new shader to showcase the fullscreen shader with.
		Tga::SpriteShader customShader; // Create	
		customShader.Init("shaders/instanced_sprite_shader_vs", "shaders/custom_sprite_pixel_shader_PS");


		float timer = 0;
		while (true)
		{
			timer += 1.0f / 60.0f;
			if (!Tga::Application::GetInstance()->BeginFrame() || !Tga::GraphicsEngine::GetInstance()->BeginFrame())
			{
				break;
			}

			Tga::Vector2ui intResolution = Tga::Application::GetInstance()->GetRenderSize();
			Tga::Vector2f resolution = { (float)intResolution.x, (float)intResolution.y };

			// Render background
			myShape.SetSize(resolution);
			Tga::GraphicsEngine::GetInstance()->GetCustomShapeDrawer().Draw(myShape);

			// Set the new sprite as a target instead of the screen
			myRenderTargetTexture.SetAsActiveTarget();
			myRenderTargetTexture.Clear();

			// set a custom camera with square aspect ratio:
			Tga::Camera camera;
			camera.SetOrtographicProjection(0.f, 1.f, 0.f, 1.f, 0.f, 1.f);
			Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack().SetCamera(camera);

			// Render all the logos onto the sprite
			Tga::GraphicsEngine::GetInstance()->GetSpriteDrawer().Draw(logoSpriteSharedData, logoInstances.data(), logoInstances.size());

			// Set the target back to the screen
			backBuffer.SetAsActiveTarget();
			Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack().SetDefaultCamera();

			// Render the target sprite which holds a lot of logos!
			{
				Tga::SpriteSharedData sharedData = {};
				sharedData.texture = &myRenderTargetTexture;
				sharedData.customShader = &customShader;

				Tga::Sprite2DInstanceData instanceData = {};
				instanceData.position = Tga::Vector2f(0.5f, 0.5f) * resolution;
				instanceData.pivot = Tga::Vector2f(0.5f, 0.5f);
				instanceData.size = Tga::Vector2f(0.7f, 0.7f) * resolution.y;
				instanceData.rotation = cosf(timer) * 0.1f;

				Tga::GraphicsEngine::GetInstance()->GetSpriteDrawer().Draw(sharedData, instanceData);
			}

			Tga::GraphicsEngine::GetInstance()->EndFrame();
			Tga::Application::GetInstance()->EndFrame();
		}
	}

	Tga::GraphicsEngine::Shutdown();
	Tga::Application::Shutdown();
}
