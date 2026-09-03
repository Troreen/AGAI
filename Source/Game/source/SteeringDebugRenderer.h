#pragma once

class Actor;
class Controller;

class SteeringDebugRenderer
{
public:
    void Draw(const Actor& aActor, const Controller& aController) const;
};
