#include "ActorManager.h"
#include "../Actor.h"
#include "../Controllers/Controller.h"

Actor& ActorManager::CreateActor(const CommonUtilities::Vector2f& aPosition, const char* aSpritePath, std::unique_ptr<Controller> aController)
{
    auto actor = std::make_unique<Actor>();
    actor->Init(aPosition, aSpritePath);
    actor->SetController(std::move(aController));
    myActors.push_back(std::move(actor));
    return *myActors.back();
}

void ActorManager::BuildNeighbourLists(CommonUtilities::Grid2D& aGrid, float aPerceptionRadius)
{
    aGrid.Clear();
    for (std::size_t actorIndex = 0; actorIndex < myActors.size(); ++actorIndex)
    {
        aGrid.InsertCircle(static_cast<int>(actorIndex), myActors[actorIndex]->GetPosition(), 0.f);
    }

    const float perceptionRadiusSqr = aPerceptionRadius * aPerceptionRadius;
    for (std::size_t actorIndex = 0; actorIndex < myActors.size(); ++actorIndex)
    {
        const Actor& actor = *myActors[actorIndex];
        std::vector<const Actor*> neighbours;
        aGrid.VisitCellsOverlappingCircle(actor.GetPosition(), aPerceptionRadius,
            [this, &aGrid, &actor, actorIndex, perceptionRadiusSqr, &neighbours](int aCellIndex)
            {
                const std::vector<int>& candidates = aGrid.GetObjectsInCell(aCellIndex);
                for (const int candidateIndex : candidates)
                {
                    if (candidateIndex == static_cast<int>(actorIndex))
                    {
                        continue;
                    }

                    const Actor* candidate = myActors[static_cast<std::size_t>(candidateIndex)].get();
                    if (actor.GetPosition().DistanceSqr(candidate->GetPosition()) <= perceptionRadiusSqr)
                    {
                        neighbours.push_back(candidate);
                    }
                }
                return true;
            });
        myActors[actorIndex]->SetNeighbours(std::move(neighbours));
    }
}

void ActorManager::Update(float aDeltaTime)
{
    for (const std::unique_ptr<Actor>& actor : myActors)
    {
        actor->Update(aDeltaTime);
    }
}

void ActorManager::Draw() const
{
    for (const auto& actor : myActors) actor->Draw();
}

Actor& ActorManager::GetActor(std::size_t aIndex) { return *myActors.at(aIndex); }
const Actor& ActorManager::GetActor(std::size_t aIndex) const { return *myActors.at(aIndex); }
std::size_t ActorManager::GetActorCount() const { return myActors.size(); }
