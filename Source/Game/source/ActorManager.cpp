#include "ActorManager.h"
#include "Actor.h"
#include "Controller.h"

Actor& ActorManager::CreateActor(const Tga::Vector2f& aPosition, const char* aSpritePath, std::unique_ptr<Controller> aController)
{
    auto actor = std::make_unique<Actor>();
    actor->Init(aPosition, aSpritePath);
    actor->SetController(std::move(aController));
    myActors.push_back(std::move(actor));
    return *myActors.back();
}

void ActorManager::Update(float aDeltaTime)
{
    for (auto& actor : myActors) actor->Update(aDeltaTime);
}

void ActorManager::Draw() const
{
    for (const auto& actor : myActors) actor->Draw();
}

Actor& ActorManager::GetActor(std::size_t aIndex) { return *myActors.at(aIndex); }
const Actor& ActorManager::GetActor(std::size_t aIndex) const { return *myActors.at(aIndex); }
std::size_t ActorManager::GetActorCount() const { return myActors.size(); }
