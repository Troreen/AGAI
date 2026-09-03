#include <tge/editor/EditorGraphics/NullEditorGraphics.h>

using namespace Tga;


void NullSceneEditorGraphics::Draw(const SceneDrawParameters& someParameters)
{
	someParameters;
}

void NullAnimationClipEditorGraphics::Draw(const AnimationClipDrawParameters& someParameters)
{
	someParameters;
}
void Tga::NullObjectDefinitionEditorGraphics::Draw(ObjectDefinitionDrawParameters& someParameters)
{
	someParameters;
}

std::unique_ptr<ObjectDefinitionEditorGraphicsBase> NullEditorGraphics::CreateObjectDefinitionGraphicsInterface() const
{
	return std::make_unique<NullObjectDefinitionEditorGraphics>();
}

std::unique_ptr<SceneEditorGraphicsBase> NullEditorGraphics::CreateSceneGraphicsInterface() const
{
	return std::make_unique<NullSceneEditorGraphics>();
}

std::unique_ptr<AnimationClipEditorGraphicsBase> NullEditorGraphics::CreateAnimationClipGraphicsInterface() const
{
	return std::make_unique<NullAnimationClipEditorGraphics>();
}
