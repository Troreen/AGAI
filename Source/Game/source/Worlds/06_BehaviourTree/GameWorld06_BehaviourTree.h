#pragma once
#include "../05_RobotWars/GameWorld05_RobotWars.h"

// Use the same arena with a third robot, rather than copying its gameplay.
class GameWorld06_BehaviourTree final : public GameWorld05_RobotWars
{
public:
    GameWorld06_BehaviourTree() : GameWorld05_RobotWars(true) {}
};
