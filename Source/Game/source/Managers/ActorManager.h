#pragma once

#include <Vector2.hpp>
#include <memory>
#include <vector>

class Actor;
class Controller;

class ActorManager
{
public:
    Actor& CreateActor(const CommonUtilities::Vector2f& aPosition, const char* aSpritePath, std::unique_ptr<Controller> aController);
    void Update(float aDeltaTime);
    void Draw() const;
    Actor& GetActor(std::size_t aIndex);
    const Actor& GetActor(std::size_t aIndex) const;
    std::size_t GetActorCount() const;

private:
    std::vector<std::unique_ptr<Actor>> myActors;
};
