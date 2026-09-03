#pragma once
#include "Controller.h"

class SeekerController final : public TargetController
{
public:
    explicit SeekerController(const TargetControllerData& aData = {});
    Tga::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    void Update(Actor& aActor, float aDeltaTime) override;
    ControllerDebugInfo GetDebugInfo() const override;
};
