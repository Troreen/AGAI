#pragma once
#include <memory>
#include <string_view>
#include <imgui/imgui.h>
#include <tge/animation/Pose.h>
#include <tge/math/Color.h>
#include <tge/math/Vector.h>
#include <unordered_map>
#include <tge/stringRegistry/StringRegistry.h>
namespace Tga
{
struct AnimationClip;
class EditorViewport;
class Scene;
class SceneSelection;
class SceneObjectDefinition;
struct LivePreviewData;

struct ObjectDefinitionDrawParameters
{
	EditorViewport* viewport;
	LivePreviewData* livePreviewData;
	SceneObjectDefinition* objectDefinition;
	StringId selectedProperty;
};

class ObjectDefinitionEditorGraphicsBase
{
public:

	virtual void Draw(ObjectDefinitionDrawParameters& parameters) = 0;
	virtual void DrawVisualPreviewSettings() {}

};

struct SceneDrawParameters
{
	EditorViewport* viewport;
	Scene* scene;
	SceneSelection* sceneSelection;
};

class SceneEditorGraphicsBase
{
public:
	virtual void Draw(const SceneDrawParameters& parameters) = 0;

};

struct AnimationClipDrawParameters
{
	EditorViewport* viewport;
	AnimationClip* clip;
	float currentTime;
	int selectedSkeletonNodeIndex;
};

class AnimationClipEditorGraphicsBase
{
public:
	virtual void Draw(const AnimationClipDrawParameters& parameters) = 0;

};


class EditorGraphicsBase
{
public:
	virtual std::unique_ptr<ObjectDefinitionEditorGraphicsBase> CreateObjectDefinitionGraphicsInterface() const = 0;
	virtual std::unique_ptr<SceneEditorGraphicsBase> CreateSceneGraphicsInterface() const = 0;
	virtual std::unique_ptr<AnimationClipEditorGraphicsBase> CreateAnimationClipGraphicsInterface() const = 0;

	virtual ImTextureID GetTextureID(std::string_view /*aTexturePath*/) const { return 0; }
	virtual void DrawLines(const Color* /*someColors*/, const Vector3f* /*someFromPositions*/, const Vector3f* /*someToPositions*/, unsigned int /*aCount*/) const {}
};

}
