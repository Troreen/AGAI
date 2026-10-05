// Change this number to run another assignment, then rebuild GameMain.
#define GAME_WORLD_ASSIGNMENT 4 // 1: Controllers, 2: Boids, 3: World Interface, 4: NavMesh

#if GAME_WORLD_ASSIGNMENT == 1
#include "Worlds/01_Controllers/GameWorld01_Controllers.h"
using SelectedGameWorld = GameWorld01_Controllers;
#elif GAME_WORLD_ASSIGNMENT == 2
#include "Worlds/02_Boids/GameWorld02_Boids.h"
using SelectedGameWorld = GameWorld02_Boids;
#elif GAME_WORLD_ASSIGNMENT == 3
#include "Worlds/03_WorldInterface/GameWorld03_WorldInterface.h"
using SelectedGameWorld = GameWorld03_WorldInterface;
#elif GAME_WORLD_ASSIGNMENT == 4
#include "Worlds/04_NavMesh/GameWorld04_NavMesh.h"
using SelectedGameWorld = GameWorld04_NavMesh;
#else
#error GAME_WORLD_ASSIGNMENT must be 1, 2, 3, or 4.
#endif

#include <tge/application.h>
#include <Input/InputHandler.h>
#include <tge/log/Log.h>
#include <tge/scene/Scene.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/settings/settings.h>

#include "tge/graphics/GraphicsEngine.h"

namespace
{
CommonUtilities::InputHandler* ourInput = nullptr; // Points to the stack-owned input handler in Go.
}

// --- Pass Windows input messages to our input handler ---
// Windows sends these when the mouse or keyboard changes, or the window closes.
LRESULT WinProc([[maybe_unused]] HWND hWnd, UINT message, [[maybe_unused]] WPARAM wParam,
                [[maybe_unused]] LPARAM lParam)
{
    // The window is created before the input handler, so wait until it exists.
    if (ourInput != nullptr)
    {
        ourInput->UpdateEvents(message, wParam, lParam);
        ourInput->UpdateMouseInput(message, wParam, lParam);
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

namespace Tga
{
void EnsureScenePropertiesAreLoaded();
void EnsureBasePropertiesAreLoaded();

void EnsureStaticInitializedTypesAreLoaded()
{
    EnsureScenePropertiesAreLoaded();
    EnsureBasePropertiesAreLoaded();
}
} // namespace Tga

// --- Start the engine and the assignment ---
void Go()
{
    Tga::EnsureStaticInitializedTypesAreLoaded();

    // Read the window size, asset folders, and other engine settings.
    Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);

    Tga::ApplicationConfiguration& cfg = Tga::Settings::GetApplicationConfiguration();

    cfg.winProcCallback = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    { return WinProc(hWnd, message, wParam, lParam); };
#ifdef _DEBUG
    cfg.activateDebugSystems = Tga::DebugFeature::Fps | Tga::DebugFeature::Mem | Tga::DebugFeature::Filewatcher |
                               Tga::DebugFeature::Cpu | Tga::DebugFeature::Drawcalls |
                               Tga::DebugFeature::OptimizeWarnings | Tga::DebugFeature::Log;
#else
    cfg.activateDebugSystems = Tga::DebugFeature::Fps | Tga::DebugFeature::Filewatcher;
#endif

    // Create the real window and renderer before creating anything in the game.
    if (!Tga::Application::Start() || !Tga::GraphicsEngine::Start())
    {
        ERROR_PRINT("Fatal error! Engine could not start!");
        system("pause");
        return;
    }

    {
        // --- Create the input handler and game world ---
        CommonUtilities::InputHandler input;
        input.SetWindowHandle(*Tga::Application::GetInstance()->GetHWND());
        ourInput = &input;
        SelectedGameWorld gameWorld;
        gameWorld.Init(input);

        Tga::Application& application = *Tga::Application::GetInstance();
        Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

        // --- The game loop: read input, update the world, then draw it ---
        // Repeat until the window is closed. Use the engine's time between frames directly.
        while (application.BeginFrame() && graphicsEngine.BeginFrame())
        {
            input.UpdateInput();
            gameWorld.Update(application.GetDeltaTime());
            gameWorld.Render();

            graphicsEngine.EndFrame();
            application.EndFrame();
        }
        ourInput = nullptr;
    }

    // --- Close the engine after the game objects have been destroyed ---
    Tga::GraphicsEngine::GetInstance()->Shutdown();
    Tga::Application::GetInstance()->Shutdown();
}
