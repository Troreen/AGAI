#include "GameWorld.h"

#include <tge/input/InputManager.h>
#include <tge/scene/Scene.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/settings/settings.h>
#include <tge/log/Log.h>
#include <tge/application.h>

#include "tge/graphics/GraphicsEngine.h"

LRESULT WinProc([[maybe_unused]]HWND hWnd, UINT message, [[maybe_unused]]WPARAM wParam, [[maybe_unused]]LPARAM lParam)
{
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


namespace Tga
{
	void EnsureScenePropertiesAreLoaded();
	void EnsureBasePropertiesAreLoaded();

	void EnsureStaticInitializedTypesAreLoaded()
	{
		EnsureScenePropertiesAreLoaded();
		EnsureBasePropertiesAreLoaded();
	}
}


void Go()
{
	Tga::EnsureStaticInitializedTypesAreLoaded();

	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);

	Tga::ApplicationConfiguration& cfg = Tga::Settings::GetApplicationConfiguration();

	cfg.winProcCallback = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {return WinProc(hWnd, message, wParam, lParam); };
#ifdef _DEBUG
	cfg.activateDebugSystems = Tga::DebugFeature::Fps | Tga::DebugFeature::Mem | Tga::DebugFeature::Filewatcher | Tga::DebugFeature::Cpu | Tga::DebugFeature::Drawcalls | Tga::DebugFeature::OptimizeWarnings | Tga::DebugFeature::Log;
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

		Tga::Application& application = *Tga::Application::GetInstance();
		Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

		while (application.BeginFrame() && graphicsEngine.BeginFrame())
		{
			gameWorld.Update(application.GetDeltaTime());
			gameWorld.Render();

			graphicsEngine.EndFrame();
			application.EndFrame();
		}
	}

	Tga::GraphicsEngine::GetInstance()->Shutdown();
	Tga::Application::GetInstance()->Shutdown();
}

