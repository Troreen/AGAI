#pragma once

#include "../GameWorld.h"

#include <memory>
#include "../../Actors/ActorManager.h"

class ITraversalBounds;

class GameWorld01_Controllers final : public GameWorld
{
public:
	GameWorld01_Controllers();
	~GameWorld01_Controllers() override;

	void Init(const CommonUtilities::InputHandler& aInput) override;
	void Update(float aTimeDelta) override;
	void Render() override;
private:
	ActorManager myActorManager;
	std::shared_ptr<ITraversalBounds> myTraversalBounds;
};
