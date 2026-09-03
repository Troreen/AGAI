#include <tge/editor/GoEditor.h>
#include <DefaultEditorGraphics.h>

int main(const int /*argc*/, const char* /*argv*/[])
{
	// Configure which game executable the editor launches when testing/playing
	EditorConfiguration configuration = {};
	configuration.debugExeName = "PlatformerGameMain_Debug.exe";
	configuration.releaseExeName = "PlatformerGameMain_Release.exe";
	configuration.debugExePath = L"..\\Bin\\PlatformerGameMain_Debug.exe";
	configuration.releaseExePath = L"..\\Bin\\PlatformerGameMain_Release.exe";

	GoEditor(TGE_PROJECT_SETTINGS_FILE, configuration, std::make_unique<Tga::DefaultEditorGraphics>());
	return 0;
}

