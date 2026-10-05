#pragma once

#include "Controller.h"

namespace CommonUtilities
{
class InputHandler;
}

class PlayerController final : public Controller
{
public:
    explicit PlayerController(const CommonUtilities::Vector2f& aStartPosition, const CommonUtilities::InputHandler& aInput);
    void Update(Actor& aActor, float aDeltaTime) override;
    CommonUtilities::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    ControllerDebugInfo GetDebugInfo() const override;
    void SetTargetPosition(const CommonUtilities::Vector2f& aPosition);

private:
    const CommonUtilities::InputHandler& myInput;
    CommonUtilities::Vector2f myTargetPosition;
};
