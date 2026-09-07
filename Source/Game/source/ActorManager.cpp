#include "ActorManager.h"
#include "Actor.h"
#include "Boid.h"
#include "Controller.h"

Actor& ActorManager::CreateActor(const Tga::Vector2f& aPosition, const char* aSpritePath, std::unique_ptr<Controller> aController)
{
    auto actor = std::make_unique<Actor>();
    actor->Init(aPosition, aSpritePath);
    actor->SetController(std::move(aController));
    myActors.push_back(std::move(actor));
    return *myActors.back();
}

Boid& ActorManager::CreateBoid(const Tga::Vector2f& aPosition, const char* aSpritePath, std::unique_ptr<Controller> aController)
{
    auto boid = std::make_unique<Boid>(std::move(aController));
    boid->Init(aPosition, aSpritePath);
    Boid* boidPointer = boid.get();
    myActors.push_back(std::move(boid));
    myBoids.push_back(boidPointer);
    return *boidPointer;
}

void ActorManager::Update(float aDeltaTime)
{
    for (Boid* boid : myBoids)
        boid->SetFlockMembers(myBoids);

    for (auto& actor : myActors) actor->Update(aDeltaTime);
}

void ActorManager::Draw() const
{
    for (const auto& actor : myActors) actor->Draw();
}

Actor& ActorManager::GetActor(std::size_t aIndex) { return *myActors.at(aIndex); }
const Actor& ActorManager::GetActor(std::size_t aIndex) const { return *myActors.at(aIndex); }
std::size_t ActorManager::GetActorCount() const { return myActors.size(); }
const std::vector<Boid*>& ActorManager::GetBoids() const { return myBoids; }
