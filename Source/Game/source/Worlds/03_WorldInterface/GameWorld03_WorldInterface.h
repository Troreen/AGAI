#pragma once

#include "../GameWorld.h"

#include "AIEventManager.h"
#include "AIPollingStation.h"
#include "../../Actors/Actor.h"
#include "../../Actors/ActorManager.h"

#include <array>
#include <cstdint>
#include <memory>

namespace CommonUtilities
{
    class InputHandler;
}

namespace Tga
{
    class Text;
} // namespace Tga

class GameWorld03_WorldInterface final : public GameWorld
{
public:
    GameWorld03_WorldInterface();
    ~GameWorld03_WorldInterface() override;

    // --- Start the scene, update it, and draw it ---
    void Init(const CommonUtilities::InputHandler& aInput) override;
    void Update(float aTimeDelta) override;
    void Render() override;

    // --- Information other parts of the game can ask for ---
    std::uint64_t GetFrameCount() const;
    const Actor* GetCurrentlyHackedComputer() const;
    const Actor* GetLatestAttemptedComputer() const;
    AIPollingStation& GetPollingStation();
    AIEventManager& GetEventManager();
    Actor& GetPlayer();
    Actor& GetGuard(std::size_t aIndex);
    const Actor& GetComputer(std::size_t aIndex) const;
    unsigned int GetStartedEventCount() const;
    unsigned int GetStoppedEventCount() const;
    float GetHackingDistance() const;

private:
    void UpdateHacking();
    void UpdateDebugUI();
    void DrawDebug();
    int GetComputerNumber(const Actor* aComputer) const;

    // --- Remember what the player is doing ---
    // A null computer pointer means there is no computer to report.
    std::uint64_t myFrameCount = 0;
    const Actor* myCurrentlyHackedComputer = nullptr;
    const Actor* myLatestAttemptedComputer = nullptr;
    float myHackingDistance = 75.f;
    unsigned int myStartedEventCount = 0;
    unsigned int myStoppedEventCount = 0;
    // --- Debug display and optional console messages ---
    bool myShowHackingDistance = true;
    bool myShowTargets = true;
    bool myLogEvents = false;
    bool myLogPolling = false;

    // --- The objects that belong to this world ---
    // C++ destroys these from bottom to top. Guards must stop listening before
    // their event manager and computers are destroyed.
    AIEventManager myEvents;
    AIPollingStation myPollingStation;
    std::array<Actor, 3> myComputers;
    ActorManager myActorManager;
    std::array<std::unique_ptr<Tga::Text>, 8> myLabels;
};
