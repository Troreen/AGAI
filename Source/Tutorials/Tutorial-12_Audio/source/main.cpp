#include <string>
#include <tge/Application.h>
#include <tge/log/Log.h>
#include <tge/settings/settings.h>
#include <tge/stringRegistry/StringRegistry.h>

#include <tge/sprite/sprite.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>

#include <tge/input/InputManager.h>
#include "../../TutorialCommon/TutorialCommon.h"
#include <tge/audio/audio.h>

using namespace std::placeholders;

Tga::InputManager* SInputManager;

struct RenderData {
	Tga::SpriteSharedData shared;
	Tga::Sprite2DInstanceData instance;
};

void Go( void );
int main( const int /*argc*/, const char * /*argc*/[] )
{
    Go();
    return 0;
}


// Run with  Left thumb stick, jump with A, shoot with X
void Go( void )
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
	{
		Tga::ApplicationConfiguration& cfg = Tga::Settings::GetApplicationConfiguration();
		cfg.winProcCallback = [](HWND, UINT message, WPARAM wParam, LPARAM lParam)
		{
			SInputManager->UpdateEvents(message, wParam, lParam);
			return 0;
		};
		TutorialCommon::Init(L"TGE: Tutorial 12, Audio");

        HWND windowHandle = *Tga::Application::GetInstance()->GetHWND();
        Tga::InputManager myInputManager(windowHandle);
        SInputManager = &myInputManager;

		Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();
		Tga::Audio audio;

		Tga::StringId music[] = { "sample1"_tgaid, "sample2"_tgaid, "sampel3"_tgaid };
		audio.Init("music.mp3", music[0]);
		audio.Init("music2.wav", music[1], false);
		audio.Init("music3.wav", music[2], false);
		audio.Init("sci-fi_weapon_blaster_laser_boom_small_02.wav", "shoot"_tgaid, false, false);
		audio.Init("S_Object_Push_Minecart_L.wav", "grind"_tgaid, false, false);

		Tga::StringId clickSound = "grind"_tgaid;
		int activeMusic = 0;

		audio.Play(music[activeMusic]);
		audio.SetVolume(music[activeMusic], 1.f);
		float timer = 0;

		Tga::Vector2ui resolution = Tga::Application::GetInstance()->GetRenderSize();
		Tga::Vector2f center = { (float)resolution.x * 0.5f, (float)resolution.y * 0.5f };
		
		RenderData tgeLogo = {};
		RenderData speaker = {};
		tgeLogo.shared.texture = graphicsEngine.GetTextureManager().GetTexture("sprites/tge_logo_b.dds");
		speaker.shared.texture = graphicsEngine.GetTextureManager().GetTexture("Speaker_Icon.dds");
		
		// Create instance data. 
		speaker.instance.pivot = Tga::Vector2f(0.5f, 0.5f);
		speaker.instance.size = { 100.f, 100.f };
		tgeLogo.instance.pivot = Tga::Vector2f(0.5f, 0.5f);
		tgeLogo.instance.size = { 300.f, 300.f };

		tgeLogo.instance.position = center + Tga::Vector2f{300.f, (float)resolution.y-300.f};
		speaker.instance.position = center + Tga::Vector2f{100.f, 100.f};

		while (true)
		{
			timer += 1.0f / 60.0f;
			if (!Tga::Application::GetInstance()->BeginFrame() || !graphicsEngine.BeginFrame())
			{
				break;
			}

            myInputManager.Update();
			if (myInputManager.IsKeyPressed(VK_SPACE)) {
				audio.Stop(music[activeMusic], true);
				activeMusic = (activeMusic+1) % ARRAYSIZE(music);
				audio.Play(music[activeMusic]);
			}

			if(myInputManager.IsKeyReleased(VK_LBUTTON))
			{
				audio.Play(clickSound, true);
				audio.SetPosition(clickSound, { 0.f,1.f, 0.f });
				tgeLogo.instance.position = { 0.f, resolution.y - 600.f };
			}

			if (audio.IsPlaying(clickSound))
			{
				float clipLen = audio.GetLengthInSeconds(clickSound);
				tgeLogo.instance.position.x += ((resolution.x+150.f) * clipLen / 60.f)/10.f;

				float x = (tgeLogo.instance.position.x / resolution.x) * 2.f - 1.f;
				audio.SetPosition(clickSound, { x, 1.f, 0.f });
			}

			Tga::Vector2f mouse = myInputManager.GetMousePosition();
			speaker.instance.position = { resolution.x/2.f, resolution.y-mouse.y };
			float scaley = 1.f - (mouse.y / resolution.y);

			speaker.instance.sizeMultiplier = { scaley, scaley };
			audio.SetVolume(music[activeMusic], scaley);

			graphicsEngine.GetSpriteDrawer().Draw(tgeLogo.shared, tgeLogo.instance);
			graphicsEngine.GetSpriteDrawer().Draw(speaker.shared, speaker.instance);
			
			graphicsEngine.EndFrame();
			Tga::Application::GetInstance()->EndFrame();
		}
	}
	Tga::GraphicsEngine::Shutdown();
	Tga::Application::GetInstance()->Shutdown();

}
