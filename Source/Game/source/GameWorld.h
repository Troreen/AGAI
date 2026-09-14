#pragma once

#include <memory>
#include <Grid2D.hpp>
#include "Managers/ActorManager.h"
#include "FlockingSettings.h"

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
	void UpdateDebugUI();
	ActorManager myActorManager;
	FlockingSettings myFlockingSettings;
	CommonUtilities::Grid2D myFlockingGrid;
	std::shared_ptr<ITraversalBounds> myTraversalBounds;
};
