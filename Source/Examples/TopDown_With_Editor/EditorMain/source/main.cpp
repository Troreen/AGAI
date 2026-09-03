#include <tge/editor/GoEditor.h>
#include <DefaultEditorGraphics.h>

int main(const int /*argc*/, const char* /*argv*/[])
{
	// set up executable paths for the editor to launch when playing/testing
	EditorConfiguration configuration = {};
	configuration.debugExeName = "TopDownGameMain_Debug.exe";
	configuration.releaseExeName = "TopDownGameMain_Release.exe";
	configuration.debugExePath = L"..\\Bin\\TopDownGameMain_Debug.exe";
	configuration.releaseExePath = L"..\\Bin\\TopDownGameMain_Release.exe";

	GoEditor(TGE_PROJECT_SETTINGS_FILE, configuration, std::make_unique<Tga::DefaultEditorGraphics>());
	return 0;
}


