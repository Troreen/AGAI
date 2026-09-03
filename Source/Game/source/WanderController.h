#pragma once

#include "Controller.h"

struct WanderControllerData : ControllerData
{
    // Radius of the circle from which the current wander target is selected.
    float circleRadius = 50.f;
    // How far ahead of the actor the wander circle is positioned.
    float circleDistance = 100.f;
    // Largest clockwise/counter-clockwise wander turn rate, in radians per second.
    float maxTurnRate = 2.5f;
    // Seconds between random turn-rate samples.
    float directionChangeInterval = 0.2f;
    // Seed for repeatable wander motion; use a distinct seed per actor.
    unsigned int randomSeed = 3u;
};

class WanderController final : public Controller
{
public:
    explicit WanderController(const WanderControllerData& aData = {});
    Tga::Vector2f GetDesiredVelocity(const Actor& aActor) const override;
    void Update(Actor& aActor, float aDeltaTime) override;
    ControllerDebugInfo GetDebugInfo() const override;

    const WanderControllerData& GetWanderControllerData() const;
    void SetWanderControllerData(const WanderControllerData& aData);

private:
    Tga::Vector2f GetWanderTarget() const;
    float GenerateTurnRate();

    Tga::Vector2f myWanderCircleCenter = {};
    float myWanderTargetAngle = 0.f;
    float myJitterTimer = 0.f;
    float myCurrentJitter = 0.f;
    WanderControllerData myWanderData;
    std::mt19937 myRandomGenerator;
};
