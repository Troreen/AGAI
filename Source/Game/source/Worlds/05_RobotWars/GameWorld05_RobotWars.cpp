#include "GameWorld05_RobotWars.h"
#include "../../Actors/Actor.h"
#include "../../Controllers/Controller.h"

#include <tge/application.h>

void GameWorld05_RobotWars::Init(const CommonUtilities::InputHandler&)
{
    const Tga::Vector2ui renderSize = Tga::Application::GetInstance()->GetRenderSize();
    const CommonUtilities::Vector2f resolution = {static_cast<float>(renderSize.x), static_cast<float>(renderSize.y)};
    myArenaBounds = std::make_shared<RectTraversalBounds>(resolution * 0.05f, resolution * 0.95f);
    CreateRobots();
    myRobotLabels[0].SetText("Decision Tree");
    myRobotLabels[1].SetText("State Machine");
}

void GameWorld05_RobotWars::CreateRobots()
{
    const CommonUtilities::Vector2f arenaSize = myArenaBounds->GetMax() - myArenaBounds->GetMin();
    const char* spritePaths[2] = {"Sprites/robot1.png", "Sprites/robot2.png"};
    for (std::size_t index = 0; index < myRobotLabels.size(); ++index)
    {
        const float horizontalFraction = index == 0 ? 0.25f : 0.75f;
        const CommonUtilities::Vector2f spawnOffset = {arenaSize.x * horizontalFraction, arenaSize.y * 0.5f};
        Actor& robot = myActorManager.CreateActor(myArenaBounds->GetMin() + spawnOffset, spritePaths[index], nullptr);
        robot.SetMaxSpeed(180.f);
        robot.SetMaxForce(1200.f);
        robot.SetMass(1.f);
        robot.SetRadius(25.f);
    }
}

void GameWorld05_RobotWars::Update(float aTimeDelta)
{
    myActorManager.Update(aTimeDelta);
}

void GameWorld05_RobotWars::Render()
{

    myActorManager.Draw();
    DrawLabels();
}

void GameWorld05_RobotWars::DrawLabels()
{
    for (std::size_t index = 0; index < myRobotLabels.size(); ++index)
    {
        const Actor& robot = myActorManager.GetActor(index);
        Tga::Text& label = myRobotLabels[index];
        label.SetPosition((robot.GetPosition() + CommonUtilities::Vector2f(-label.GetWidth() * 0.5f, 45.f)).ToTga());
        label.Render();
    }
}
