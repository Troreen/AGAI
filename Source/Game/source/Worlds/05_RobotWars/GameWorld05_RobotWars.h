#pragma once

#include "../GameWorld.h"
#include "../../Actors/Actor.h"
#include "../../Actors/ActorManager.h"
#include "../../Interfaces/TraversalBounds.h"
#include <tge/text/text.h>
#include <array>
#include <memory>

class GameWorld05_RobotWars final : public GameWorld
{
public:
    void Init(const CommonUtilities::InputHandler& aInput) override;
    void Update(float aTimeDelta) override;
    void Render() override;

private:
    void CreateRobots();

    void DrawLabels();

    ActorManager myActorManager;
    std::shared_ptr<RectTraversalBounds> myArenaBounds;
    std::array<Tga::Text, 2> myRobotLabels;

};
