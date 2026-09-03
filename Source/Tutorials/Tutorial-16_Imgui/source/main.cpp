#include <string>
#include <tge/Application.h>
#include <tge/log/Log.h>

#include "../../TutorialCommon/TutorialCommon.h"
#include <tge/audio/audio.h>
#include <tge/imgui/ImGuiInterface.h>
#include <tge/settings/settings.h>

#include <imgui/imgui.h>

void Go(void);
int main(const int /*argc*/, const char* /*argc*/[])
{
	Go();
	return 0;
}

LRESULT WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	lParam;
	wParam;
	hWnd;
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

void Go()
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
	Tga::ApplicationConfiguration& cfg = Tga::Settings::GetApplicationConfiguration();
	{
#ifdef _RETAIL 
		MessageBoxA(nullptr, "Imgui is not avalible in retail, select release or debug instead", "Error", MB_OK);
#endif
		// The below code is the same as the start of the default Tge start. You can find this code in "Game.cpp" under the Game project
		cfg.winProcCallback = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {return WinProc(hWnd, message, wParam, lParam); };

		cfg.applicationName = L"TGE: Tutorial 16 - Imgui";
		//cfg.preferedMultiSamplingQuality = Tga::MultiSamplingQuality::High;
		cfg.activateDebugSystems = Tga::DebugFeature::Fps |
			Tga::DebugFeature::Mem |
			Tga::DebugFeature::Drawcalls |
			Tga::DebugFeature::Cpu |
			Tga::DebugFeature::Filewatcher |
			Tga::DebugFeature::OptimizeWarnings;

		if (!Tga::Application::Start() || !Tga::GraphicsEngine::Start())
		{
			ERROR_PRINT("Fatal error! Engine could not start!");
			system("pause");
			return;
		}

		Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();
		Tga::Application& engine = *Tga::Application::GetInstance();
		while (true)
		{
			if (!engine.BeginFrame() || !graphicsEngine.BeginFrame()) {
				break;
			}
#ifndef _RETAIL // <-- SUPER IMPORTANT as imgui is not compiled in retail, if you dont have this you will get a compiler error on all imgui calls you do
			
			ImGui::ShowDemoWindow();

			//Below we change the back color
			static bool isOpen = false;
			if (ImGui::Begin("Settings", &isOpen, 0))
			{

				static float rgb[3];
				ImGui::ColorPicker3("BG Color", rgb);

				Tga::Application::GetInstance()->SetClearColor(Tga::Color(rgb[0], rgb[1], rgb[2], 1.0f));

			}
			ImGui::End();
#endif
			graphicsEngine.EndFrame();
			engine.EndFrame();
		}
	}
	Tga::GraphicsEngine::Shutdown();
	Tga::Application::Shutdown();
}

