#include <tge/editor/GoEditor.h>
#include <DefaultEditorGraphics.h>

int main(const int /*argc*/, const char* /*argc*/[])
{
	EditorConfiguration configuration = {};
	configuration.enableVisualScripts = true;
	configuration.debugExeName = "AnimationGameMain_Debug.exe";
	configuration.releaseExeName = "AnimationGameMain_Release.exe";
	configuration.debugExePath = L"..\\Bin\\AnimationGameMain_Debug.exe";
	configuration.releaseExePath = L"..\\Bin\\AnimationGameMain_Release.exe";

	GoEditor(TGE_PROJECT_SETTINGS_FILE, configuration, std::make_unique<Tga::DefaultEditorGraphics>());
	return 0;
}
