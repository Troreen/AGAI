#pragma once

namespace CommonUtilities { class InputHandler; }

// Every assignment has the same three steps. Its own class keeps its scene data.
class GameWorld
{
public:
    virtual ~GameWorld();
    virtual void Init(const CommonUtilities::InputHandler& aInput) = 0;
    virtual void Update(float aTimeDelta) = 0;
    virtual void Render() = 0;
};
