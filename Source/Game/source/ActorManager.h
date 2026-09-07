#pragma once

#include <tge/math/vector.h>
#include <memory>
#include <vector>

class Actor;
class Boid;
class Controller;

class ActorManager
{
public:
    Actor& CreateActor(const Tga::Vector2f& aPosition, const char* aSpritePath, std::unique_ptr<Controller> aController);
    Boid& CreateBoid(const Tga::Vector2f& aPosition, const char* aSpritePath, std::unique_ptr<Controller> aController);
    void Update(float aDeltaTime);
    void Draw() const;
    Actor& GetActor(std::size_t aIndex);
    const Actor& GetActor(std::size_t aIndex) const;
    std::size_t GetActorCount() const;
    const std::vector<Boid*>& GetBoids() const;

private:
    std::vector<std::unique_ptr<Actor>> myActors;
    std::vector<Boid*> myBoids;
};
