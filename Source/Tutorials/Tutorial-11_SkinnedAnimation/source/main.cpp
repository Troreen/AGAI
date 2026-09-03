#include <string>
#include <tge/Application.h>
#include <tge/log/Log.h>

#include <tge/input/XInput.h>
#include "../../TutorialCommon/TutorialCommon.h"
#include <tge/audio/audio.h>
#include <tge/animation/AnimationPlayer.h>
#include <tge/graphics/Camera.h>
#include <tge/graphics/dx11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/model/modelfactory.h>
#include <tge/stringRegistry/StringRegistry.h>

#include <tge/Timer.h>
#include <tge/settings/settings.h>

using namespace std::placeholders;
using namespace Tga;
void Go( void );
int main( const int /*argc*/, const char * /*argc*/[] )
{
    Go();
    return 0;
}

/* This function will trigger as soon as an animation is sending an event from the animation, could be anything! Footsteps, something spawning, anything really
The animators will add these events in spine, and set what should be send, maybe a string, maybe a float or integer, thats why there are so many in the function.
*/
void AnimCallback()
{
	
}

// Run with  Left thumb stick, jump with A, shoot with X
void Go(void)
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
	{
		TutorialCommon::Init(L"TGE: Tutorial 11");

		/* See Audio-tutorial for examples of using sounds
		Tga::Audio myFootstepAudios[4];
		Tga::Audio myShootAudio;

		myFootstepAudios[0].Init("audio/footstep_wood_run_01.wav", "footstep_wood_run_1"_tgaid);
		myFootstepAudios[1].Init("audio/footstep_wood_run_02.wav", "footstep_wood_run_2"_tgaid);
		myFootstepAudios[2].Init("audio/footstep_wood_run_03.wav", "footstep_wood_run_3"_tgaid);
		myFootstepAudios[3].Init("audio/footstep_wood_run_04.wav", "footstep_wood_run_4"_tgaid);
		
		myShootAudio.Init("audio/sci-fi_weapon_blaster_laser_boom_small_02.wav", "weapon"_tgaid);
		*/

		Tga::GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

		// Create the sprite with the path to the image
		ModelFactory& modelFactory = ModelFactory::GetInstance();
		Tga::AnimatedModelInstance model = modelFactory.GetAnimatedModelInstance("ani/popp_sk.fbx");
		model.GetTransform().SetPosition({0.0f, -0.25f, 0.5f});
		model.GetTransform().Scale(Vector3f(0.1f, 0.1f, 0.1f));
		XInput myInput;

		std::vector<AnimationPlayer> animations = 
		{ 
			modelFactory.GetAnimationPlayer("ani/sneak.fbx", model.GetModel()->GetSkeleton()),
			modelFactory.GetAnimationPlayer("ani/idle.fbx", model.GetModel()->GetSkeleton()),
			modelFactory.GetAnimationPlayer("ani/run.fbx", model.GetModel()->GetSkeleton()),
			modelFactory.GetAnimationPlayer("ani/jump.fbx", model.GetModel()->GetSkeleton())
		};
		animations[0].SetIsLooping(true);
		animations[1].SetIsLooping(true);
		animations[2].SetIsLooping(true);

		// TODO: DB 2022-04-07 Disabled events for the time being due to change in import stack being rushed. Will be reimplemented as an upgrade.
		//model.RegisterAnimationEventCallback("event_step", [] { myFootstepAudios[rand() % 3].Play(); });

		TextureResource* texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("ani/atlas.tga");
		model.SetTexture(0, 0, texture);

		const int IDLE_INDEX = 1;
		const int RUNNING_INDEX = 2;
		const int JUMPING_INDEX = 3;
		Timer aTimer;

		int activeAnimationIndex = IDLE_INDEX;
		animations[IDLE_INDEX].Play();

		while (true)
		{
			if (!Tga::Application::GetInstance()->BeginFrame() || !graphicsEngine.BeginFrame())
			{
				break;
			}

			Vector2ui resolution = DX11::GetResolution();
			Tga::Camera camera;
			camera.SetOrtographicProjection(1.0, resolution.y / (float)resolution.x, 1.0f);
			Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack().SetCamera(camera);

			myInput.Refresh();

			if (activeAnimationIndex != JUMPING_INDEX)
			{
				if (myInput.leftStickX > 0.2f)
				{
					activeAnimationIndex = RUNNING_INDEX;
					animations[activeAnimationIndex].Play();
					model.GetTransform().ResetScaleAndRotation();
					model.GetTransform().Scale(Vector3f(-0.1f, 0.1f, -0.1f));
					
				}
				else if (myInput.leftStickX < -0.2f)
				{
					activeAnimationIndex = RUNNING_INDEX;
					animations[activeAnimationIndex].Play();
					model.GetTransform().ResetScaleAndRotation();
					model.GetTransform().Scale(Vector3f(0.1f,0.1f,-0.1f));
				}
				else
				{
					activeAnimationIndex = IDLE_INDEX;
					animations[activeAnimationIndex].Play();
				}
			}
			else if (activeAnimationIndex == JUMPING_INDEX)
			{
				if (animations[activeAnimationIndex].GetState() == AnimationState::Finished)
				{
					activeAnimationIndex = IDLE_INDEX;
					animations[activeAnimationIndex].Play();
				}
			}

			if (myInput.IsPressed(XINPUT_GAMEPAD_A))
			{
				if (activeAnimationIndex != JUMPING_INDEX)
				{
					activeAnimationIndex = JUMPING_INDEX;
					animations[activeAnimationIndex].Play();
				}

			}

			aTimer.Update();
			animations[activeAnimationIndex].Update(aTimer.GetDeltaTime());
			model.SetPose(animations[activeAnimationIndex]);

			Tga::GraphicsEngine::GetInstance()->GetModelDrawer().Draw(model);

			graphicsEngine.EndFrame();
			Tga::Application::GetInstance()->EndFrame();
		}
	}
	Tga::GraphicsEngine::Shutdown();
	Tga::Application::GetInstance()->Shutdown();

}
