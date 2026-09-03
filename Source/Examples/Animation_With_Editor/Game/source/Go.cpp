#include "GameWorld.h"

#include <tge/application.h>
#include <tge/input/InputManager.h>
#include <tge/scene/Scene.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/settings/settings.h>
#include <tge/log/Log.h>

#include "tge/graphics/GraphicsEngine.h"

Tga::InputManager* SInputManager;

LRESULT WinProc(HWND /*hWnd*/, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (SInputManager->UpdateEvents(message, wParam, lParam)) {
		return 0;
	}

	switch (message)
	{
		// this message is read when the window is closed
		case WM_DESTROY:
		{
			// close the application entirely
			PostQuitMessage(0);
			return 0;
		}
	}

	return 0;
}


void Go(const char* aSceneToLoad)
{
	aSceneToLoad;

	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);

	Tga::ApplicationConfiguration &cfg = Tga::Settings::GetApplicationConfiguration();
	
	cfg.winProcCallback = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {return WinProc(hWnd, message, wParam, lParam); };
#ifdef _DEBUG
	cfg.activateDebugSystems = Tga::DebugFeature::Fps 
		| Tga::DebugFeature::Mem 
		| Tga::DebugFeature::Filewatcher 
		| Tga::DebugFeature::Cpu 
		| Tga::DebugFeature::Drawcalls 
		| Tga::DebugFeature::OptimizeWarnings;
#else
	cfg.activateDebugSystems = Tga::DebugFeature::Filewatcher;
#endif

	if (!Tga::Application::Start() || !Tga::GraphicsEngine::Start())
	{
		ERROR_PRINT("Fatal error! Engine could not start!");
		system("pause");
		return;
	}
	
	{
		GameWorld gameWorld;
		gameWorld.Init();

		Tga::Application& engine = *Tga::Application::GetInstance();
		Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

		Tga::InputManager inputManager(*engine.GetHWND());
		SInputManager = &inputManager;

		while (engine.BeginFrame() && graphicsEngine.BeginFrame())
		{
			inputManager.Update();
			gameWorld.Update(engine.GetDeltaTime(), inputManager);
			gameWorld.Render();

			graphicsEngine.EndFrame();
			engine.EndFrame();
		}
	}

	Tga::GraphicsEngine::GetInstance()->Shutdown();
	Tga::Application::GetInstance()->Shutdown();
}

