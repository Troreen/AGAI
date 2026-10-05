#include "GameWorld03_WorldInterface.h"

#include "../../Controllers/Steering/PlayerController.h"
#include "../../Controllers/WorldInterface/WorldInterfacingControllers.h"

#include <tge/application.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/text/text.h>
#ifndef _RETAIL
#include <imgui/imgui.h>
#endif

#include <cassert>
#include <cstdio>

namespace
{
const char* guardNames[4] = {"Smart / Poll", "Smart / Event", "Stupid / Poll", "Stupid / Event"};
}

GameWorld03_WorldInterface::GameWorld03_WorldInterface() : myPollingStation(*this)
{
}

GameWorld03_WorldInterface::~GameWorld03_WorldInterface() = default;

// --- Set up the playing field ---
// Create the computers, player, guards, and their name labels.
void GameWorld03_WorldInterface::Init(const CommonUtilities::InputHandler& aInput)
{
    assert(myActorManager.GetActorCount() == 0);
    const Tga::Vector2ui size = Tga::Application::GetInstance()->GetRenderSize();
    const CommonUtilities::Vector2f resolution = {static_cast<float>(size.x), static_cast<float>(size.y)};

    // --- Place the three computers ---
    // Triangle formation, with its bounds centred.
    const CommonUtilities::Vector2f centre = resolution * 0.5f;
    const CommonUtilities::Vector2f spread = {resolution.x * 0.2f, resolution.y * 0.25f};
    const CommonUtilities::Vector2f computerPositions[3] = {{centre.x - spread.x, centre.y - spread.y},
                                                            {centre.x + spread.x, centre.y},
                                                            {centre.x - spread.x, centre.y + spread.y}};
    for (std::size_t index = 0; index < myComputers.size(); ++index)
    {
        myComputers[index].Init(computerPositions[index], "Sprites/computer.png");
    }

    // --- Create the mouse-controlled player ---
    // The controller decides where to go; the Actor handles the actual movement.
    const CommonUtilities::Vector2f playerPosition = {resolution.x * 0.5f, resolution.y * 0.55f};
    Actor& player = myActorManager.CreateActor(playerPosition, "Sprites/human.png",
                                               std::make_unique<PlayerController>(playerPosition, aInput));
    player.SetMaxSpeed(300.f);
    player.SetMaxForce(1200.f);
    player.SetMass(0.15f);

    // --- Create the four guards ---
    // Each pair follows the same rule, but one asks for information and one gets messages.
    myActorManager.CreateActor({resolution.x * 0.18f, resolution.y * 0.45f},
                               "Sprites/robot1.png",
                               std::make_unique<SmartGuardPollController>(myPollingStation));
    myActorManager.CreateActor({resolution.x * 0.22f, resolution.y * 0.60f},
                               "Sprites/robot1.png",
                               std::make_unique<SmartGuardEventController>(myEvents));
    myActorManager.CreateActor({resolution.x * 0.70f, resolution.y * 0.78f},
                               "Sprites/robot2.png",
                               std::make_unique<StupidGuardPollController>(myPollingStation));
    myActorManager.CreateActor({resolution.x * 0.80f, resolution.y * 0.82f},
                               "Sprites/robot2.png",
                               std::make_unique<StupidGuardEventController>(myEvents));

    // --- Give the guards different colours and matching movement speeds ---
    const Tga::Color colors[4] = {
        {0.3f, 1.f, 1.f, 1.f}, {0.65f, 0.75f, 1.f, 1.f}, {1.f, 0.8f, 0.3f, 1.f}, {1.f, 0.4f, 0.4f, 1.f}};
    for (std::size_t index = 0; index < 4; ++index)
    {
        Actor& guard = GetGuard(index);
        guard.SetMaxSpeed(160.f);
        guard.SetMaxForce(1200.f);
        guard.SetMass(0.15f);
        guard.SetColor(colors[index]);
    }

    // --- Give everything a name on screen ---
    const char* labels[8] = {"Player (left-click to move)",
                             guardNames[0],
                             guardNames[1],
                             guardNames[2],
                             guardNames[3],
                             "Computer 1",
                             "Computer 2",
                             "Computer 3"};
    for (std::size_t index = 0; index < myLabels.size(); ++index)
    {
        myLabels[index] = std::make_unique<Tga::Text>("Text/arial.ttf", Tga::FontSize_14);
        myLabels[index]->SetText(labels[index]);
    }
}

// --- Run one game frame ---
// A frame is one turn of the game loop. The frame number is used by the polling station.
void GameWorld03_WorldInterface::Update(float aTimeDelta)
{
    ++myFrameCount;
    // Move the player first, then check hacking, then move the guards.
    // This lets both kinds of guard react to the same information in this frame.
    GetPlayer().Update(aTimeDelta);
    UpdateHacking();
    for (std::size_t index = 0; index < 4; ++index)
    {
        GetGuard(index).Update(aTimeDelta);
    }

    UpdateDebugUI();
}

// --- Work out which computer the player is hacking ---
// Being close enough is all that is needed;
void GameWorld03_WorldInterface::UpdateHacking()
{
    // No computer is selected until we find one within reach.
    const Actor* hackedComputer = nullptr;
    // DistanceSqr gives distance times distance, so compare with reach times reach too.
    const float distanceSqr = myHackingDistance * myHackingDistance;
    for (const Actor& computer : myComputers)
    {
        if (GetPlayer().GetPosition().DistanceSqr(computer.GetPosition()) <= distanceSqr)
        {
            hackedComputer = &computer;
            break;
        }
    }

    // Staying at the same computer (or staying away) is not a new event.
    if (hackedComputer == myCurrentlyHackedComputer)
    {
        return; // No state change means no event this frame.
    }

    // Remember what changed. Smart guards still need the latest attempt after leaving.
    const Actor* previousComputer = myCurrentlyHackedComputer;
    myCurrentlyHackedComputer = hackedComputer;
    if (hackedComputer != nullptr)
    {
        myLatestAttemptedComputer = hackedComputer; // Remember after the player leaves.
    }

    // Tell listeners that hacking stopped at the old computer.
    // When switching computers, this message goes out before the new start message.
    if (previousComputer != nullptr)
    {
        ++myStoppedEventCount;
        myEvents.SendEvent(PlayerStoppedHackingEvent{previousComputer});
        if (myLogEvents)
        {
            std::printf("[AI frame %llu] PlayerStoppedHacking: computer %d\n", myFrameCount,
                        GetComputerNumber(previousComputer));
        }
    }
    // Tell listeners once when the player starts hacking the new computer.
    if (hackedComputer != nullptr)
    {
        ++myStartedEventCount;
        myEvents.SendEvent(PlayerStartedHackingEvent{hackedComputer});
        if (myLogEvents)
        {
            std::printf("[AI frame %llu] PlayerStartedHacking: computer %d\n", myFrameCount,
                        GetComputerNumber(hackedComputer));
        }
    }
}

// --- Show the small debug panel ---
// These options help us watch the game; they do not change the guard rules.
void GameWorld03_WorldInterface::UpdateDebugUI()
{
#ifndef _RETAIL
    if (!ImGui::Begin("U03 - World Interfacing", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Currently hacking: %d (0 = none)", GetComputerNumber(myCurrentlyHackedComputer));
    ImGui::Text("Latest attempt: %d", GetComputerNumber(myLatestAttemptedComputer));

    if (ImGui::CollapsingHeader("Visuals"))
    {
        ImGui::Checkbox("Show hacking ranges", &myShowHackingDistance);
        ImGui::Checkbox("Show target lines", &myShowTargets);
    }

    if (ImGui::CollapsingHeader("Events and polling"))
    {
        ImGui::Text("Frame: %llu", myFrameCount);
        ImGui::Text("Events: started %u / stopped %u", myStartedEventCount, myStoppedEventCount);
        const AIPollingCache& current = myPollingStation.GetCurrentCacheDebug();
        const AIPollingCache& latest = myPollingStation.GetLatestCacheDebug();
        ImGui::Text("Current: requests %llu / refreshes %llu", current.GetRequestCount(), current.GetRefreshCount());
        ImGui::Text("Latest: requests %llu / refreshes %llu", latest.GetRequestCount(), latest.GetRefreshCount());
        ImGui::Checkbox("Print events", &myLogEvents);
        if (ImGui::Checkbox("Print polling refreshes", &myLogPolling))
        {
            myPollingStation.SetLogRefreshes(myLogPolling);
        }
    }
    ImGui::End();
#endif
}

// --- Draw the game ---
// Actors draw their sprites. We also draw computer colours and name labels.
void GameWorld03_WorldInterface::Render()
{
    myActorManager.Draw();
    for (std::size_t index = 0; index < myComputers.size(); ++index)
    {
        Actor& computer = myComputers[index];
        // Green means hacking now, yellow means the latest old attempt, white means neither.
        computer.SetColor(&computer == myCurrentlyHackedComputer
                              ? Tga::Color(0.3f, 1.f, 0.4f, 1.f)
                              : (&computer == myLatestAttemptedComputer ? Tga::Color(1.f, 0.85f, 0.3f, 1.f)
                                                                        : Tga::Color(1, 1, 1, 1)));
        computer.Draw();
    }

    for (std::size_t index = 0; index < myLabels.size(); ++index)
    {
        const Actor& actor = index < 5 ? myActorManager.GetActor(index) : myComputers[index - 5];
        const float labelOffset = (index == 2 || index == 4) ? 50.f : 32.f;
        myLabels[index]->SetColor(actor.GetSpriteInstanceData().color);
        myLabels[index]->SetPosition((actor.GetPosition() + CommonUtilities::Vector2f{-30.f, labelOffset}).ToTga());
        myLabels[index]->Render();
    }
    DrawDebug();
}

// --- Draw optional guides on the playing field ---
// Circles show hacking reach. Lines show where the player and guards want to go.
void GameWorld03_WorldInterface::DrawDebug()
{
#ifndef _RETAIL
    Tga::DebugDrawer& drawer = Tga::GraphicsEngine::GetInstance()->GetDebugDrawer();
    if (myShowHackingDistance)
    {
        for (const Actor& computer : myComputers)
        {
            drawer.DrawCircle(computer.GetPosition().ToTga(), myHackingDistance, Tga::Color(0.4f, 1.f, 0.4f, 1.f));
        }
    }
    for (std::size_t index = 0; index < myActorManager.GetActorCount(); ++index)
    {
        const Actor& actor = myActorManager.GetActor(index);
        const ControllerDebugInfo info = actor.GetController()->GetDebugInfo();
        if (myShowTargets && info.hasTarget)
        {
            drawer.DrawLine(actor.GetPosition().ToTga(), info.targetPosition.ToTga(),
                            actor.GetSpriteInstanceData().color);
        }
    }
#endif
}

int GameWorld03_WorldInterface::GetComputerNumber(const Actor* aComputer) const
{
    for (std::size_t index = 0; index < myComputers.size(); ++index)
    {
        if (aComputer == &myComputers[index])
        {
            return static_cast<int>(index) + 1;
        }
    }
    return 0;
}

std::uint64_t GameWorld03_WorldInterface::GetFrameCount() const
{
    return myFrameCount;
}
const Actor* GameWorld03_WorldInterface::GetCurrentlyHackedComputer() const
{
    return myCurrentlyHackedComputer;
}
const Actor* GameWorld03_WorldInterface::GetLatestAttemptedComputer() const
{
    return myLatestAttemptedComputer;
}
AIPollingStation& GameWorld03_WorldInterface::GetPollingStation()
{
    return myPollingStation;
}
AIEventManager& GameWorld03_WorldInterface::GetEventManager()
{
    return myEvents;
}
Actor& GameWorld03_WorldInterface::GetPlayer()
{
    return myActorManager.GetActor(0);
}
Actor& GameWorld03_WorldInterface::GetGuard(std::size_t aIndex)
{
    assert(aIndex < 4);
    return myActorManager.GetActor(aIndex + 1);
}
const Actor& GameWorld03_WorldInterface::GetComputer(std::size_t aIndex) const
{
    return myComputers.at(aIndex);
}
unsigned int GameWorld03_WorldInterface::GetStartedEventCount() const
{
    return myStartedEventCount;
}
unsigned int GameWorld03_WorldInterface::GetStoppedEventCount() const
{
    return myStoppedEventCount;
}
float GameWorld03_WorldInterface::GetHackingDistance() const
{
    return myHackingDistance;
}
