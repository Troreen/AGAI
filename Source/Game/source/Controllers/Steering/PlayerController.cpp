#include "PlayerController.h"
#include "../../Actors/Actor.h"
#include "../ControllerUtils.h"

#include <tge/graphics/DX11.h>
#include <Input/InputHandler.h>

#ifndef _RETAIL
#include <imgui/imgui.h>
#endif

// --- Remember the input handler and the player's starting destination ---
PlayerController::PlayerController(const CommonUtilities::Vector2f& aStartPosition, const CommonUtilities::InputHandler& aInput)
    : myInput(aInput), myTargetPosition(aStartPosition)
{
}

// --- Read a click and choose where the player should go ---
void PlayerController::Update(Actor& aActor, float)
{
    // Clicking the debug panel should not also send the player walking somewhere.
    bool acceptMouseInput = true;
#ifndef _RETAIL
    if (ImGui::GetIO().WantCaptureMouse)
    {
        acceptMouseInput = false;
    }
#endif
    if (acceptMouseInput && myInput.IsMouseButtonPressed(Keys::MOUSELBUTTON))
    {
        const POINT mousePosition = myInput.GetMousePos();
        // Mouse coordinates start at the top-left; world coordinates start at the bottom-left.
        SetTargetPosition({static_cast<float>(mousePosition.x),
                           static_cast<float>(Tga::DX11::GetResolution().y) - static_cast<float>(mousePosition.y)});
    }

    // Stop when we are close enough, instead of circling around the clicked point.
    if (aActor.GetPosition().DistanceSqr(myTargetPosition) <= 4.f * 4.f)
    {
        aActor.SetPosition(myTargetPosition);
        aActor.Stop();
    }
}

// --- Choose a direction and slow down near the destination ---
// Actor handles moving the player, just as it does for the guards.
CommonUtilities::Vector2f PlayerController::GetDesiredVelocity(const Actor& aActor) const
{
    return ControllerUtils::ArriveDesiredVelocity(aActor, myTargetPosition, 80.f);
}

// --- Give the debug drawing the player's destination ---
ControllerDebugInfo PlayerController::GetDebugInfo() const
{
    ControllerDebugInfo debugInfo;
    debugInfo.hasTarget = true;
    debugInfo.targetPosition = myTargetPosition;
    return debugInfo;
}

// --- Remember the latest clicked position ---
const CommonUtilities::Vector2f& PlayerController::GetTargetPosition() const
{
    return myTargetPosition;
}

void PlayerController::SetTargetPosition(const CommonUtilities::Vector2f& aPosition)
{
    myTargetPosition = aPosition;
}
