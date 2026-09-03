#include <string>

#include <tge/drawers/DebugDrawer.h>
#include <tge/math/vector2.h>
#include <tge/Application.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include "../../TutorialCommon/TutorialCommon.h"

#include <tge/settings/settings.h>
#include <tge/log/Log.h>
#include <cstdlib>

void Go( void );
int main( const int /*argc*/, const char * /*argc*/[] )
{
    Go();

    return 0;
}


void Go( void )
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
	{
		TutorialCommon::Init(L"TGE: Tutorial 8", true);


		auto winsize = Tga::Application::GetInstance()->GetRenderSize();

		const float deltaTime = 1.0f / 60.0f;
		float timer = 0.0f;
		while (true)
		{
			timer += deltaTime;
			if (!Tga::Application::GetInstance()->BeginFrame() || !Tga::GraphicsEngine::GetInstance()->BeginFrame())
			{
				break;
			}

			if (rand() % 60 == 0) 
			{
				int type = rand() % 4;
				if (type == 0)
				{
					INFO_PRINT("Random log message index: %d", rand() % 10);
				}
				else if (type == 1)
				{
					INFO_TIP("Random tip message index: %d", rand() % 10);
				}
				else if (type == 2)
				{
					ERROR_PRINT("Random error message index: %d", rand() % 10);
				}
				else
				{
					ERROR_PRINT("Showcasing duplicate error message!");
				}
			}

			// Arrow
			{
				Tga::Vector2f from(0.2f*winsize.x, 0.8f*winsize.y);
				Tga::Vector2f to(0.4f*winsize.x, 0.6f*winsize.y);
				Tga::Color color(1, 0, 0, 1);
				Tga::GraphicsEngine::GetInstance()->GetDebugDrawer().DrawArrow(from, to, color, 100.f);
			}

			//Circle
			{
				Tga::GraphicsEngine::GetInstance()->GetDebugDrawer().DrawCircle({ 0.3f*winsize.x, 0.4f*winsize.y}, 30.0f + (cosf(timer) * 0.1f), Tga::Color(cosf(timer), -cosf(timer * 3), sinf(timer), 1));
			}

			//Lines
			struct Line
			{
				Tga::Vector2f myFrom;
				Tga::Vector2f myTo;
			};

			// Single Line
			{
				Line line;
				line.myFrom.Set(0.5f*winsize.x, 0.1f*winsize.y);
				line.myTo.Set(0.2f*winsize.x, 0.3f*winsize.y);
				Tga::GraphicsEngine::GetInstance()->GetDebugDrawer().DrawLine(line.myFrom, line.myTo, { 1, 1, 1, 1 });
			}

			// Hierarchy visualization
			// GraphicsStateStack allows changing coordinate system and camera
			// GetDebugDrawer will use this
			{
				Tga::GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();

				graphicsStateStack.Push();
				graphicsStateStack.Translate({ 0.5f * winsize.x, 0.5f * winsize.y , 0.f });

				int count = 5;
				for (int i = 0; i < count; i++)
				{
					graphicsStateStack.Rotate(Tga::Vector3f{ 0.f, 0.f, 180/ count * sinf((0.25f * timer + i/(float)count)*3.1415f) });

					Tga::GraphicsEngine::GetInstance()->GetDebugDrawer().DrawLine({ 0, 0 }, { 200.f/ count, 0 }, { 1, 0, 0, 1 });
					Tga::GraphicsEngine::GetInstance()->GetDebugDrawer().DrawLine({ 0, 0 }, { 0, 200.f / count }, { 0, 1, 0, 1 });

					graphicsStateStack.Translate({ 500.f/ count, 0.f, 0.f });

				}

				graphicsStateStack.Pop();
			}

			Tga::GraphicsEngine::GetInstance()->EndFrame();
			Tga::Application::GetInstance()->EndFrame();
		}

		Tga::GraphicsEngine::Shutdown();
		Tga::Application::Shutdown();
	}
}
