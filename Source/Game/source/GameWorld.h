#pragma once

#include <memory>
#include "ActorManager.h"

class ITraversalBounds;

class GameWorld
{
public:
	GameWorld(); 
	~GameWorld();

	void Init();
	void Update(float aTimeDelta);
	void Render();
private:
	ActorManager myActorManager;
	std::shared_ptr<ITraversalBounds> myTraversalBounds;
};
