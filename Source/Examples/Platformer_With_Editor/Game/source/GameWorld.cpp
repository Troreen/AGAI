#include "GameWorld.h"

#include <tge/application.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/animation/AnimationPlayer.h>
#include <tge/drawers/DebugDrawer.h>

#include <tge/scene/Scene.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/RenderTarget.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/Model/AnimatedModelInstance.h>
#include <tge/Model/ModelInstance.h>
#include <tge/Model/ModelFactory.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/graphics/Camera.h>
#include <tge/sprite/Sprite.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/script/BaseProperties.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <tge/settings/settings.h>
#include <tge/math/Matrix4x4.h>
#include <tge/input/InputManager.h>
#include <tge/Timer.h>

#include <tge/text/TextService.h>
#include <tge/text/text.h>

#include <tge/script/Nodes/CommonNodes.h>
#include <tge/script/Nodes/ExampleNodes.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/SceneObjectNodes.h>

constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;

using namespace Tga;

GameWorld::GameWorld()
{}

GameWorld::~GameWorld() 
{}

// Callback function type triggered when interacting with in-world switches
typedef void (*ActiveFunc)(bool aIsActive);

struct InstanceData {
	Tga::Matrix4x4f transform;
	ModelInstance instance;
};

// Represents an interactive switch/lever placed in the level
struct SwitchData {
	InstanceData instanceData;
	Tga::Vector3f position;
	Tga::Text text;
	float radius = 0.0f;
	bool active = false;
	ActiveFunc activeFunc = nullptr;
};

struct RenderData {
	std::vector<InstanceData> instances;
	std::vector<SwitchData> switches;
	AnimatedModelInstance player;
	Tga::SpriteSharedData cloudSpriteSharedData;
	Tga::Text* activeInfoText = nullptr;
};

struct CameraData {
	Tga::Camera sideScrolling;
	Tga::Camera freeFly;

	Vector3f smoothCameraOffset;
	float smoothLookOffset = 0.0f;
	Vector2i capturedCameraPos;

	float camSpeed = 1000.f;
	float camRotSpeed = 1.f;
	Vector3f freeFlyCameraRotation;

	bool isFreeFly = false;
	bool isCursorHidden = false;
};

enum class PlayerState
{
	Idle,
	Walking,
	Jumping,
	Poking
};

enum class PlayerDirection
{
	Left,
	Right,
};

struct PlayerData
{
	PlayerState state = PlayerState::Idle;
	PlayerDirection direction = PlayerDirection::Right;
	AnimationPlayer idleAnimation;
	AnimationPlayer runAnimation;
	AnimationPlayer jumpAnimation;
	AnimationPlayer pokeAnimation;
	float rotation = 0.0f;
	float cameraOffset = 0.0f;
};

static RenderData locRenderData;
static PlayerData locPlayerData;
static CameraData locCameraData;
static Timer locTimer;

static std::unordered_map<Tga::StringId, ActiveFunc> locActiveFunctions;

// Register callback functions attached to switches in the level scene
void SetupActiveFunctions()
{
	// Switch effect 1: Animates alpha test threshold to show how alpha testing cuts away sprite textures
	locActiveFunctions["alpha_threshold"_tgaid] = [](bool aIsActive)
		{
		GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();
		GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();
			SpriteDrawer& spriteDrawer = graphicsEngine.GetSpriteDrawer();

			graphicsStateStack.Push();

			float t = 0.f;
			if(aIsActive)
			{
				t = 3.f * (float)locTimer.GetTotalTime();
				graphicsStateStack.SetAlphaTestThreshold(0.5f + 0.45f * sinf(t));
			}
			else
			{
				graphicsStateStack.SetAlphaTestThreshold(0.9f);
			}

			Vector3f position = { 0.f, 300.f, -200.f };
			float size = 400.f;
			Tga::Sprite3DInstanceData instance = {};
		instance.transform = Matrix4x4f::CreateFromScale(size);
		instance.transform.SetPosition(position + Vector3f{ -0.5f * size, 0.5f * size, 0.f });
		instance.color = { 0.7f, 0.4f, 0.1f, 1.0f };
			spriteDrawer.Draw(locRenderData.cloudSpriteSharedData, instance);

			graphicsStateStack.Pop();
		};

	// Switch effect 2: Renders orbiting 3D sprites demonstrating spatial sprite drawing
	locActiveFunctions["juggle_example"_tgaid] = [](bool aIsActive)
		{
		GraphicsEngine& graphicsEngine = *Tga::GraphicsEngine::GetInstance();
		GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();
			SpriteDrawer& spriteDrawer = graphicsEngine.GetSpriteDrawer();

			graphicsStateStack.SetAlphaTestThreshold(0.f);

			Tga::Sprite3DInstanceData instance = {};
		instance.color = { 0.7f, 0.4f, 0.1f, 1.0f };
			float size = 300.f;

			if (aIsActive)
			{	
				const int numinstances = 10;
				for (int i = 1; i < numinstances + 1; i++)
				{
					float t = (float)locTimer.GetTotalTime() + 2.f * 3.1415f * (float)i / numinstances;
				Vector3f position = { 100.f * cosf(t), 150.f + 100.f * sinf(2.f * (float)t), 300.f + 5.f * i };

				instance.transform = Matrix4x4f::CreateFromScale(size);
				instance.transform.SetPosition(Vector3f{ -0.5f * size, 0.5f * size, 0.f });
				instance.transform *= Matrix4x4f::CreateRotationAroundZ(i + 100.f * (float)locTimer.GetTotalTime());
				instance.transform.Translate(position);
				instance.color = { 0.9f, 0.3f, 0.3f, 1.0f };

						spriteDrawer.Draw(locRenderData.cloudSpriteSharedData, instance);
					}
				}
			else
			{
			instance.transform = Matrix4x4f::CreateFromScale(size);
			instance.transform.SetPosition(Vector3f{ -0.5f * size, 0.5f * size + 300.f, -300.f });
				spriteDrawer.Draw(locRenderData.cloudSpriteSharedData, instance);
			}
		};

	// Switch effect 3: Alpha blending demo
	locActiveFunctions["alpha_blended_sprite"_tgaid] = [](bool aIsActive)
		{
		GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();
			graphicsStateStack.SetBlendState(BlendState::AlphaBlend);
			locActiveFunctions["juggle_example"_tgaid](aIsActive);
		};

	// Switch effect 4: Additive blending demo
	locActiveFunctions["additive_blend_sprite"_tgaid] = [](bool aIsActive)
		{
		GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();
			graphicsStateStack.SetBlendState(BlendState::AdditiveBlend);
			locActiveFunctions["juggle_example"_tgaid](aIsActive);
		};
}

void GameWorld::LoadScene(Tga::Scene& aScene)
{
	GraphicsEngine& engine = *Tga::GraphicsEngine::GetInstance();
	ModelFactory& modelFactory = ModelFactory::GetInstance();
	TextureManager& textureManager = engine.GetTextureManager();

	SetupActiveFunctions();

	std::vector<ScenePropertyDefinition> sceneObjectProperties;
	Tga::Font font48 = engine.GetTextService().GetOrLoad("Text/arial.ttf", Tga::FontSize_48);
	locRenderData.cloudSpriteSharedData.texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("sprites/cloud.dds");

	// This is just an example to parse the scene and show it
	// All you get is data, so it is up to you to apply your own representation when loading
	// Using "p.second->GetName()" is typically not the best way to identify and most times it is better to
	// use a property. Name is typically thought of as a somewhat unique so for example when duplicating an object,
	// a number is automatically added. example: if we duplicate an object named: "Foo" the copy will be named "Foo (1)"
	// I used it here, mainly to show and hopefully to show that there is some information in the scene objects, and to
	// access the properties we need to calculate or construct the property set.

	// Iterate through all objects placed in the GoEditor level scene (.leveldata)
	for (const auto& p : aScene.GetSceneObjects())
	{
		sceneObjectProperties.clear();
		p.second->CalculateCombinedPropertySet(mySceneObjectDefinitionManager, sceneObjectProperties);

		// Find player spawn point and initialize animated player character
		if (Tga::StringRegistry().RegisterOrGetString(p.second->GetName()) == "PlayerSpawn"_tgaid)
		{
			locRenderData.player = ModelFactory::GetInstance().GetAnimatedModelInstance("character/popp_sk.fbx");
			locRenderData.player.SetTransform(p.second->GetTransform());
			locRenderData.player.SetTexture(0, 0, Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("character/atlas.tga"));

			// Load animation clips defined on the PlayerSpawn object
			for (ScenePropertyDefinition& property : sceneObjectProperties)
			{
				if (property.name == "idle"_tgaid)
				{
					const AnimationClipReference& clip = property.value.Get<CopyOnWriteWrapper<AnimationClipReference>>()->Get();
					locPlayerData.idleAnimation = modelFactory.GetAnimationPlayer(clip.path.GetString(), locRenderData.player.GetModel()->GetSkeleton());
				}
				else if (property.name == "run"_tgaid)
				{
					const AnimationClipReference& clip = property.value.Get<CopyOnWriteWrapper<AnimationClipReference>>()->Get();
					locPlayerData.runAnimation = modelFactory.GetAnimationPlayer(clip.path.GetString(), locRenderData.player.GetModel()->GetSkeleton());
				}
				else if (property.name == "jump"_tgaid)
				{
					const AnimationClipReference& clip = property.value.Get<CopyOnWriteWrapper<AnimationClipReference>>()->Get();
					locPlayerData.jumpAnimation = modelFactory.GetAnimationPlayer(clip.path.GetString(), locRenderData.player.GetModel()->GetSkeleton());
				}
				else if (property.name == "poke"_tgaid)
				{
					const AnimationClipReference& clip = property.value.Get<CopyOnWriteWrapper<AnimationClipReference>>()->Get();
					locPlayerData.pokeAnimation = modelFactory.GetAnimationPlayer(clip.path.GetString(), locRenderData.player.GetModel()->GetSkeleton());
				}
			}

			locPlayerData.idleAnimation.SetIsLooping(true);
			locPlayerData.runAnimation.SetIsLooping(true);

			locPlayerData.idleAnimation.Play();
			locPlayerData.runAnimation.Play();

			locPlayerData.rotation = 0.f;
			locPlayerData.direction = p.second->GetTransform().GetRight().Dot({ 0.f, 1.f, 0.f }) <= 0.f ? PlayerDirection::Right : PlayerDirection::Left;

			float scale = 100.f;
			locRenderData.player.GetTransform().Scale(Vector3f(scale, scale, scale));
			
			continue;
		}

		for (ScenePropertyDefinition& property : sceneObjectProperties)
		{
			if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneModel>>())
			{
				const SceneModel& value = property.value.Get<CopyOnWriteWrapper<SceneModel>>()->Get();

				StringId path = value.path;
				FilePathStream dummyPath;
				if (path.IsEmpty() || !Settings::ResolveAssetPath(path, dummyPath))
					continue;

				if (ModelFactory::GetInstance().GetModel(path.GetString()))
				{
					ModelInstance instance = ModelFactory::GetInstance().GetModelInstance(path.GetString());
					int meshCount = (int)instance.GetModel()->GetMeshCount();
					if (meshCount > MAX_MESHES_PER_MODEL)
						meshCount = MAX_MESHES_PER_MODEL;

					for (int i = 0; i < meshCount; i++)
					{
						for (int j = 0; j < 4; j++)
						{
							if (!value.textures[i][j].IsEmpty())
							{
								// Diffuse texture is in sRGB, other maps (normal, material) are linear
								TextureSrgbMode srgbMode = (j == 0) ? TextureSrgbMode::ForceSrgbFormat : TextureSrgbMode::ForceNoSrgbFormat;
								Texture* texture = textureManager.GetTexture(value.textures[i][j].GetString(), srgbMode);

								if (texture != nullptr)
									instance.SetTexture(i, j, texture);
							}
						}
					}

					// Check if this object is an interactive switch
					if (Tga::StringRegistry().RegisterOrGetString(p.second->GetName()) == "switch"_tgaid)
					{
						auto & b = locRenderData.switches.emplace_back();
						b.position = p.second->GetTransform().GetPosition();
						b.instanceData = { p.second->GetTransform(), instance };

						sceneObjectProperties.clear();
						p.second->CalculateCombinedPropertySet(mySceneObjectDefinitionManager, sceneObjectProperties);

						for (ScenePropertyDefinition& objproperty : sceneObjectProperties)
						{
							if (objproperty.name == "radius"_tgaid)
							{
								b.radius = *objproperty.value.Get<float>();
							}
							else if (objproperty.name == "active"_tgaid)
							{
								b.active = *objproperty.value.Get<bool>();
								auto r = b.instanceData.transform.CreateRotationAroundZ(b.active ? -30.f : 30.f);
								b.instanceData.transform.Rotate(r.GetRotationAsQuaternion());
							}
							else if (objproperty.name == "text"_tgaid)
							{
								b.text = Tga::Text(font48);
								std::string text = objproperty.value.Get<StringId>()->GetString();
								size_t pos = 0;
								while ((pos = text.find("\\n", pos)) != std::string::npos) {
									text.replace(pos, 2, "\n");
									pos += 1;
								}

								b.text.SetText(text.c_str());
								b.text.SetPosition({ -0.5f * b.text.GetWidth(), 50.f });
							}
							else if (objproperty.name == "active_func"_tgaid)
							{
								b.activeFunc = locActiveFunctions[*objproperty.value.Get<StringId>()];
							}
						}
					}
					else
					{
						// Static environment prop
						locRenderData.instances.push_back({ p.second->GetTransform(), instance });
					}
				}
			}
			else if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneSprite>>())
			{
				const SceneSprite& value = property.value.Get<CopyOnWriteWrapper<SceneSprite>>()->Get();

				SpriteSharedData sharedData = {};
				sharedData.texture = textureManager.GetTexture(value.textures[0].GetString(), TextureSrgbMode::ForceSrgbFormat);
				sharedData.maps[0] = textureManager.GetTexture(value.textures[1].GetString(), TextureSrgbMode::ForceNoSrgbFormat);
				sharedData.maps[1] = textureManager.GetTexture(value.textures[2].GetString(), TextureSrgbMode::ForceNoSrgbFormat);
				sharedData.maps[2] = textureManager.GetTexture(value.textures[3].GetString(), TextureSrgbMode::ForceNoSrgbFormat);

				Sprite2DInstanceData instance = {};
				instance.pivot = value.pivot;
				instance.size = value.size;
				Tga::GraphicsEngine::GetInstance()->GetSpriteDrawer().Draw(sharedData, instance);
			}
		}
	}

	locPlayerData.cameraOffset = (locRenderData.player.GetTransform().GetPosition() - locCameraData.sideScrolling.GetTransform().GetPosition()).Length();
}

void GameWorld::Init()
{
	mySceneObjectDefinitionManager.Init(Settings::GameAssetRoot().c_str());

	Tga::Vector2ui resolution = Tga::Application::GetInstance()->GetRenderSize();
	
	// Setup 2D side-scrolling perspective camera
	locCameraData.sideScrolling.SetPerspectiveProjection(
		90,
		{
			(float)resolution.x,
			(float)resolution.y
		},
		0.1f,
		50000.0f
	);

	// Setup 6-DOF freefly debug camera (toggled with TAB)
	locCameraData.freeFlyCameraRotation = Rotator(30, 0, 0);
		locCameraData.freeFly.SetPerspectiveProjection(
			90,
			{
				(float)resolution.x,
				(float)resolution.y
			},
			0.1f,
		50000.0f
	);
		locCameraData.freeFly.GetTransform().SetRotation(locCameraData.freeFlyCameraRotation);

	// Custom drop shadow shader and unit cube proxy used to project shadows onto ground geometry
	myDropShadowShader.Init("shaders/model_shader_VS", "shaders/DropShadowPS");
	myShadowCubeInstance = Tga::ModelFactory::GetInstance().GetUnitCube();
}

void GameWorld::Update(float /*aTimeDelta*/, InputManager& inputManager)
{
	locTimer.Update();
	if (inputManager.IsKeyPressed(VK_ESCAPE))
	{
		PostQuitMessage(0);
	}

	// Toggle between side-scroller camera and freefly debug camera with TAB
	{
		if (inputManager.IsKeyPressed(VK_TAB))
		{
			if (!locCameraData.isFreeFly)
			{
				locCameraData.freeFly = locCameraData.sideScrolling;
				locCameraData.isFreeFly = true;
			}
			else
			{
				locCameraData.isFreeFly = false;
			}
		}

		// Handle freefly camera controls (WASD + mouse look + Shift sprint)
		HWND windowHandle = *Tga::Application::GetInstance()->GetHWND();
		if (locCameraData.isFreeFly && GetForegroundWindow() == windowHandle)
		{
			if (!locCameraData.isCursorHidden)
			{
				POINT cursorPos;
				GetCursorPos(&cursorPos);

				locCameraData.capturedCameraPos.x = cursorPos.x;
				locCameraData.capturedCameraPos.y = cursorPos.y;

				inputManager.HideMouse();
				inputManager.CaptureMouse();

				locCameraData.isCursorHidden = true;
			}

			SetCursorPos(locCameraData.capturedCameraPos.x, locCameraData.capturedCameraPos.y);

			Vector3f camMovement;
			Vector3f camRotation;

			if (inputManager.IsKeyHeld(0x57)) // W
			{
				camMovement += locCameraData.freeFly.GetTransform().GetForward() * 1.0f;
			}
			if (inputManager.IsKeyHeld(0x53)) // S
			{
				camMovement += locCameraData.freeFly.GetTransform().GetForward() * -1.0f;
			}
			if (inputManager.IsKeyHeld(0x41)) // A
			{
				camMovement += locCameraData.freeFly.GetTransform().GetRight() * -1.0f;
			}
			if (inputManager.IsKeyHeld(0x44)) // D
			{
				camMovement += locCameraData.freeFly.GetTransform().GetRight() * 1.0f;
			}

			locCameraData.freeFly.GetTransform().SetPosition(locCameraData.freeFly.GetTransform().GetPosition() + camMovement * locCameraData.camSpeed * locTimer.GetDeltaTime());

			const Vector2f mouseDelta = inputManager.GetMouseDelta();
			camRotation.X = mouseDelta.Y;
			camRotation.Y = mouseDelta.X;

			locCameraData.freeFlyCameraRotation += camRotation * locCameraData.camRotSpeed;
			locCameraData.freeFly.GetTransform().SetRotation(locCameraData.freeFlyCameraRotation);

			if (inputManager.IsKeyPressed(VK_SHIFT))
			{
				locCameraData.camSpeed *= 4;
			}
			if (inputManager.IsKeyReleased(VK_SHIFT))
			{
				locCameraData.camSpeed /= 4;
			}
		}
		else
		{
			if (locCameraData.isCursorHidden)
			{
				inputManager.ShowMouse();
				inputManager.ReleaseMouse();
				locCameraData.isCursorHidden = false;
			}
		}
	}

	// Handle player character movement, animation state machine, and switch interactions
	{
		bool isTryingToJump = false;
		bool isTryingToMoveLeft = false;
		bool isTryingToMoveRight = false;
		bool isTryingToPoke = false;

			if (!locCameraData.isFreeFly)
			{
				if (inputManager.IsKeyHeld('W') || inputManager.IsKeyHeld(' '))
					isTryingToJump = true;
				if (inputManager.IsKeyHeld('A'))
					isTryingToMoveLeft = true;
				if (inputManager.IsKeyHeld('D'))
					isTryingToMoveRight = true;
				if (inputManager.IsKeyHeld('E'))
				{
					isTryingToPoke = true;
					isTryingToJump = false;
				}

				if (isTryingToMoveLeft && isTryingToMoveRight)
				{
					isTryingToMoveLeft = false;
					isTryingToMoveRight = false;
				}
			}

		// Animation frame adjustments: skip initial setup frames for snappier responsive gameplay
		{
			if (isTryingToJump)
			{
				if (locPlayerData.jumpAnimation.GetState() != AnimationState::Playing)
				{
					locPlayerData.jumpAnimation.SetFrame(5); // Skip start of jump animation
					locPlayerData.jumpAnimation.Play();
				}
			}
			if (locPlayerData.jumpAnimation.GetFrame() > 17)
				locPlayerData.jumpAnimation.Stop();

			if (isTryingToPoke)
			{
				if (locPlayerData.pokeAnimation.GetState() != AnimationState::Playing)
				{
					locPlayerData.pokeAnimation.SetFrame(0);
					locPlayerData.pokeAnimation.Play();
				}
			}
			if (locPlayerData.pokeAnimation.GetFrame() > 15)
			{
				locPlayerData.pokeAnimation.Stop();
			}

			// Check proximity to interactive switches; flip switch on poke frame 3
			{
				locRenderData.activeInfoText = nullptr;
				for (SwitchData& switchInstance : locRenderData.switches)
				{
					Vector3f triggerpoint = locRenderData.player.GetTransform().GetPosition() - locRenderData.player.GetTransform().GetRight() * 2.9f;
					const float distance_sqr = (triggerpoint - switchInstance.position).LengthSqr();
					if (distance_sqr < switchInstance.radius * switchInstance.radius)
					{
						locRenderData.activeInfoText = &switchInstance.text;
						constexpr int triggerframe = 3;
						if (locPlayerData.pokeAnimation.GetFrame() == triggerframe)
						{
							switchInstance.active = !switchInstance.active;
							auto& data = switchInstance.instanceData;
							auto r = data.transform.CreateRotationAroundZ(switchInstance.active ? -60.f : 60.f);
							data.transform.Rotate(r.GetRotationAsQuaternion());
							locPlayerData.pokeAnimation.SetFrame(triggerframe+1);
						}
					}
				}
			}
		}

		// Update position, scale flipping, and smooth camera tracking
		{
			bool isMoving = false;
			float rotationSpeed = 30.f;

			Tga::Vector3f camerapos = locCameraData.sideScrolling.GetTransform().GetPosition();
			Tga::Vector3f playerpos = locRenderData.player.GetTransform().GetPosition();
			Tga::Vector3f cameraforward = locCameraData.sideScrolling.GetTransform().GetForward();

			Tga::Vector3f rotationTarget{ 0.f };
			if (isTryingToMoveLeft || isTryingToMoveRight)
			{
				isMoving = true;
				float ypos = locRenderData.player.GetTransform().GetPosition().y;

				locRenderData.player.GetTransform() = Matrix4x4f::CreateIdentityMatrix();

				if (isTryingToMoveLeft)
				{
					locPlayerData.direction = PlayerDirection::Left;
					locRenderData.player.GetTransform().Scale({ 100.f, 100.f, -100.f });
					rotationTarget.y = -1.f * locTimer.GetDeltaTime() * rotationSpeed;
				}
				if (isTryingToMoveRight)
				{
					locPlayerData.direction = PlayerDirection::Right;
					locRenderData.player.GetTransform().Scale({ -100.f, 100.f, -100.f });
					rotationTarget.y = 1.f * locTimer.GetDeltaTime() * rotationSpeed;
				}
				locRenderData.player.GetTransform().SetPosition({ 0.f, ypos, locPlayerData.cameraOffset });

				locPlayerData.rotation += rotationTarget.y;
				locRenderData.player.GetTransform() *= Matrix4x4f::CreateRotationAroundY(locPlayerData.rotation);
			}			

			if (locPlayerData.jumpAnimation.GetState() == AnimationState::Playing)
			{
				locPlayerData.state = PlayerState::Jumping;
			}
			else if (locPlayerData.pokeAnimation.GetState() == AnimationState::Playing)
			{
				locPlayerData.state = PlayerState::Poking;
			}
			else
			{
				if (locPlayerData.state != PlayerState::Walking && isMoving)
				{
					locPlayerData.runAnimation.SetFrame(20); // Align run animation foot plant
					locPlayerData.state = PlayerState::Walking;
				}
				if (locPlayerData.state != PlayerState::Idle && !isMoving)
				{
					locPlayerData.idleAnimation.SetFrame(0);
					locPlayerData.state = PlayerState::Idle;
				}
			}

			// Exponential camera smoothing: smooths jump offsets and lookahead direction
			Vector3f cameraOffset;
			locCameraData.sideScrolling.GetTransform() = Matrix4x4f::CreateIdentityMatrix();
			
			if (locPlayerData.state == PlayerState::Jumping)
			{
				cameraOffset += Vector3f(0.f, 200.f, 0.f);
			}

			{
				float secondsToMoveHalfDistance = 0.25f;
				float k = powf(0.5f, locTimer.GetDeltaTime() / secondsToMoveHalfDistance);
				locCameraData.smoothCameraOffset = locCameraData.smoothCameraOffset + (1 - k) * (cameraOffset - locCameraData.smoothCameraOffset);
			}

			{
				float lookOffset = (locPlayerData.direction == PlayerDirection::Left ? -25.f : 25.f);
				float secondsToMoveHalfDistance = 1.f;
				float k = powf(0.5f, locTimer.GetDeltaTime() / secondsToMoveHalfDistance);
				locCameraData.smoothLookOffset = locCameraData.smoothLookOffset + (1 - k) * (lookOffset - locCameraData.smoothLookOffset);
			}

			locCameraData.sideScrolling.GetTransform() *= Matrix4x4f::CreateRotationAroundY(locPlayerData.rotation + locCameraData.smoothLookOffset);
			locCameraData.sideScrolling.GetTransform().SetPosition(locCameraData.smoothCameraOffset + Vector3f(0.0f, 300.f, 0.0f));
		}

		// Update animation players and apply skeleton poses
		{
			locPlayerData.idleAnimation.Update(locTimer.GetDeltaTime());
			locPlayerData.runAnimation.Update(locTimer.GetDeltaTime());
			locPlayerData.jumpAnimation.Update(locTimer.GetDeltaTime());
			locPlayerData.pokeAnimation.Update(locTimer.GetDeltaTime());

			switch (locPlayerData.state)
			{
			case PlayerState::Idle:
				locRenderData.player.SetPose(locPlayerData.idleAnimation);
				break;
			case PlayerState::Walking:
				locRenderData.player.SetPose(locPlayerData.runAnimation);
				break;
			case PlayerState::Jumping:
				locRenderData.player.SetPose(locPlayerData.jumpAnimation);
				break;
			case PlayerState::Poking:
				locRenderData.player.SetPose(locPlayerData.pokeAnimation);
				break;
			default:
				break;
			}
		}
	}
}

void GameWorld::Render()
{
	auto& graphics = *Tga::GraphicsEngine::GetInstance();
	auto& graphicsStateStack = graphics.GetGraphicsStateStack();
	const Camera& renderCamera = locCameraData.isFreeFly ? locCameraData.freeFly : locCameraData.sideScrolling;

	constexpr float DropShadowRadius = 150.0f;

		graphicsStateStack.Push();
		{
			graphicsStateStack.SetCamera(renderCamera);

		// ---------------------------------------------------------------------
		// 1. Draw ground & environment tiles (writes depth buffer)
		// ---------------------------------------------------------------------
		DX11::BackBuffer->SetAsActiveTarget(DX11::DepthBuffer);
				graphicsStateStack.SetBlendState(Tga::BlendState::Disabled);
		graphicsStateStack.SetDepthStencilState(DepthStencilState::WriteLess);
				graphicsStateStack.SetAlphaTestThreshold(0.5f);

				for (const InstanceData& object : locRenderData.instances)
				{
					graphicsStateStack.SetTransform(object.transform);
			graphics.GetModelDrawer().Draw(object.instance);
				}

		Matrix4x4f groundTransform = graphicsStateStack.GetTransform();

		// ---------------------------------------------------------------------
		// 2. Projected Drop Shadow Pass:
		// Uses a unit cube volume proxy and samples the depth buffer from step 1
		// to project a soft circular shadow onto whatever geometry is below the player
		// ---------------------------------------------------------------------
		DX11::BackBuffer->SetAsActiveTarget(); // Unbind depth buffer from writing so it can be read

		graphicsStateStack.SetBlendState(Tga::BlendState::AlphaBlend);
		graphicsStateStack.SetRasterizerState(RasterizerState::BackfaceCulling);
		graphicsStateStack.SetCustomShaderParameters(Tga::Vector4f(DropShadowRadius, 0.6f, 0.0f, 0.0f));

		DX11::DepthBuffer->SetAsResourceOnSlot(8);

		Matrix4x4f playerWorldTransform = locRenderData.player.GetTransform() * groundTransform;
		Tga::Vector3f playerWorldPos = playerWorldTransform.GetPosition();

		// GetUnitCube has size 100 (extents -50 to +50), scale by (DropShadowRadius / 50)
		Tga::Matrix4x4f cubeTransform = Tga::Matrix4x4f::CreateFromScale(DropShadowRadius / 50.0f);
		cubeTransform.SetPosition(playerWorldPos);
		graphicsStateStack.SetTransform(cubeTransform);

		graphics.GetModelDrawer().Draw(myShadowCubeInstance, myDropShadowShader);

		ID3D11ShaderResourceView* nullView = nullptr;
		DX11::Context->PSSetShaderResources(8, 1, &nullView);

		// ---------------------------------------------------------------------
		// 3. Draw player, interactive switches, and switch visual effects
		// ---------------------------------------------------------------------
		DX11::BackBuffer->SetAsActiveTarget(DX11::DepthBuffer);
		graphicsStateStack.SetBlendState(Tga::BlendState::Disabled);
		graphicsStateStack.SetRasterizerState(RasterizerState::NoFaceCulling);
		graphicsStateStack.SetDepthStencilState(DepthStencilState::WriteLess);
		graphicsStateStack.SetAlphaTestThreshold(0.5f);

		graphicsStateStack.SetTransform(groundTransform);
		graphics.GetModelDrawer().Draw(locRenderData.player);

			graphicsStateStack.SetDepthStencilState(DepthStencilState::WriteLessOrEqual);

			for (SwitchData& switchInstance : locRenderData.switches)
			{
			InstanceData& data = switchInstance.instanceData;
				graphicsStateStack.Push();
				graphicsStateStack.SetTransform(data.transform);
			graphics.GetModelDrawer().Draw(data.instance);
				graphicsStateStack.Pop();

				if (switchInstance.activeFunc)
				{
					Matrix4x4f t;
				t.SetForward(-1.f * switchInstance.instanceData.transform.GetForward());
					t.SetRight(Vector3f::Up.Cross(t.GetForward()));
					t.SetPosition(switchInstance.position);
					graphicsStateStack.SetTransform(t);
					switchInstance.activeFunc(switchInstance.active);
				}
			}

		// ---------------------------------------------------------------------
		// 4. Render floating prompt text when near an interactive switch
		// ---------------------------------------------------------------------
			if (locRenderData.activeInfoText)
			{
				graphicsStateStack.Push();
				{
					graphicsStateStack.SetTransform(locCameraData.sideScrolling.GetTransform());
					graphicsStateStack.Translate({ 0.f, -150.f, 400.f });
					graphicsStateStack.Scale(0.25f);

					locRenderData.activeInfoText->Render();
				}
				graphicsStateStack.Pop();
			}
		}
		graphicsStateStack.Pop();
		DX11::BackBuffer->SetAsActiveTarget();
	}
