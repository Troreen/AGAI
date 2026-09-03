#include <string>
#include <tge/Application.h>
#include <tge/log/Log.h>
#include <tge/settings/settings.h>

#include <tge/input/InputManager.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/sprite/sprite.h>
#include <tge/texture/TextureManager.h>
#include <tge/drawers/SpriteDrawer.h>

#include "../../TutorialCommon/TutorialCommon.h"
#include "tge/videoplayer/video.h"
#include "tge/videoplayer/VideoAudio.h"

using namespace std::placeholders;

Tga::InputManager* SInputManager;

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
	Tga::ApplicationConfiguration& cfg = Tga::Settings::GetApplicationConfiguration();

    cfg.winProcCallback = [](HWND, UINT message, WPARAM wParam, LPARAM lParam)
    {
        SInputManager->UpdateEvents(message, wParam, lParam);
        return 0;
    };
	TutorialCommon::Init(L"TGE: Tutorial 13, Video");
	{

		Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();
		Tga::SpriteDrawer& spriteDrawer(graphicsEngine.GetSpriteDrawer());

        HWND windowHandle = *Tga::Application::GetInstance()->GetHWND();
        Tga::InputManager myInputManager(windowHandle);
        SInputManager = &myInputManager;

        Tga::Video video;
        video.Init("tga.mp4", true);
        video.Play(false);

        Tga::Vector2ui resolution = Tga::Application::GetInstance()->GetRenderSize();
        Tga::Vector2f center = { (float)resolution.x * 0.5f, (float)resolution.y * 0.5f };

        Tga::SpriteSharedData sharedData = {};
        sharedData.texture = video.GetTexture();

        Tga::Sprite2DInstanceData spriteInstance = {};
        spriteInstance.position = center;
        spriteInstance.size = { 500.f };
        spriteInstance.pivot = Tga::Vector2f(0.5f, 0.5f);

        bool pause = false;
        while (true)
        {
			if (!Tga::Application::GetInstance()->BeginFrame() || !graphicsEngine.BeginFrame())
            {
                break;
            }

            myInputManager.Update();
            float dt = Tga::Application::GetInstance()->GetDeltaTime();
            if (pause == false)
            {
				video.Update(dt);
            }

            if (myInputManager.IsKeyPressed(VK_SPACE)) {
                pause = !pause;
            }

            if (myInputManager.IsKeyReleased('R')) {
                video.Stop();
                video.Restart();
            }


            spriteDrawer.Draw(sharedData, spriteInstance);

			graphicsEngine.EndFrame();
			Tga::Application::GetInstance()->EndFrame();
        }
	}
	Tga::GraphicsEngine::Shutdown();
	Tga::Application::GetInstance()->Shutdown();
}
