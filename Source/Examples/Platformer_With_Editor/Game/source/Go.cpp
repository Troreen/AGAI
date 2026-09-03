#include "GameWorld.h"

#include <tge/input/InputManager.h>
#include <tge/scene/Scene.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/settings/settings.h>
#include <tge/log/Log.h>
#include <tge/application.h>
#include <tge/graphics/GraphicsEngine.h>

Tga::InputManager* SInputManager = nullptr;

LRESULT WinProc(HWND /*hWnd*/, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (SInputManager && SInputManager->UpdateEvents(message, wParam, lParam)) {
		return 0;
	}

	switch (message)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	return 0;
}

void Go(const char* aSceneToLoad)
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);

	Tga::ApplicationConfiguration &cfg = Tga::Settings::GetApplicationConfiguration();
	cfg.winProcCallback = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { return WinProc(hWnd, message, wParam, lParam); };
	
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
		return;
	}
	
	{
		GameWorld gameWorld;
		gameWorld.Init();

		// Load the level scene passed from the editor or fall back to default
		Tga::Scene scene;
		if (aSceneToLoad != nullptr)
		{
			Tga::LoadScene(aSceneToLoad, scene);
		}
		else
		{
			Tga::LoadScene("scene.leveldata", scene);
		}
		
		gameWorld.LoadScene(scene);

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


