#pragma once
#include <tge/Application.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/log/Log.h>

namespace TutorialCommon
{
	void Init(std::wstring aAppName, bool aFullDebug = false, bool aVsyncEnable = true)
	{
		unsigned short windowWidth = 1920;
		unsigned short windowHeight = 1080;

		Tga::ApplicationConfiguration& cfg = Tga::Settings::GetApplicationConfiguration();
		if (aFullDebug)
		{
			cfg.activateDebugSystems = Tga::DebugFeature::Fps | Tga::DebugFeature::Mem | Tga::DebugFeature::Filewatcher | Tga::DebugFeature::Cpu | Tga::DebugFeature::Drawcalls | Tga::DebugFeature::OptimizeWarnings | Tga::DebugFeature::Log;

		}
		else
		{
			cfg.activateDebugSystems = Tga::DebugFeature::Filewatcher;
		}

		cfg.windowSize = { windowWidth, windowHeight };
		cfg.renderSize = { windowWidth, windowHeight };
		cfg.startInFullScreen = false;
		cfg.preferedMultiSamplingQuality = Tga::MultiSamplingQuality::High;

		cfg.applicationName = aAppName;
		cfg.enableVSync = aVsyncEnable;

		if( !Tga::Application::Start() || !Tga::GraphicsEngine::Start())
		{
			ERROR_PRINT("Fatal error! Engine could not start!");
			system("pause");
	
		}
	}
}