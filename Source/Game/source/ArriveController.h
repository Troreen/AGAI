#pragma once

#include "Controller.h"

struct ArriveControllerData : TargetControllerData
{
    // Inside this distance, desired speed decreases linearly towards zero.
    float slowingRadius = 100.f;
    // Seconds before the arrive controller selects another random target.
    float targetChangeInterval = 5.f;
};

class ArriveController final : public TargetController
{
public:
    explicit ArriveController(const ArriveControllerData& aData = {});
    Tga::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    void Update(Actor& aActor, float aDeltaTime) override;
    ControllerDebugInfo GetDebugInfo() const override;

    const ArriveControllerData& GetArriveControllerData() const;
    void SetArriveControllerData(const ArriveControllerData& aData);

private:
    float myTargetTimer = 0.f;
    ArriveControllerData myArriveData;
};
