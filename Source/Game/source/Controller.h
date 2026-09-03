#pragma once

#include <tge/math/vector.h>
#include <memory>
#include <random>

class Actor;
class ITraversalBounds;

struct ControllerData
{
    // Multiplier for the controller's main steering force (seek, arrive, or wander).
    float behaviorWeight = 1.f;
    // Strength of predictive edge correction while the actor is still in bounds.
    float containmentWeight = 3.f;
    // Seconds of velocity to project forward when checking future boundary violations.
    float boundaryLookAhead = 0.75f;
    // Extra gap kept between the actor's edge and traversal bounds.
    float boundaryClearance = 20.f;
    // Enables the shared traversal-bounds behavior for this controller.
    bool useContainment = true;
};

struct TargetControllerData : ControllerData
{
    // Distance from the target at which a seeker chooses a new random target.
    float targetReachedRadius = 25.f;
    // Seed for repeatable random target selection; use a distinct seed per actor.
    unsigned int randomSeed = 2u;
};

struct ControllerDebugInfo
{
    bool hasTarget = false;
    Tga::Vector2f targetPosition = {};
    float slowingRadius = 0.f;
    bool hasSlowingRadius = false;
    float wanderCircleRadius = 0.f;
    float wanderCircleDistance = 0.f;
    Tga::Vector2f wanderCircleCenter = {};
    float wanderTargetAngle = 0.f;
};

struct ContainmentDebugInfo
{
    bool isEnabled = false;
    bool needsCorrection = false;
    Tga::Vector2f predictedPosition = {};
    Tga::Vector2f nearestValidPoint = {};
    float actorRadius = 0.f;
    float boundaryClearance = 0.f;
};

class Controller
{
public:
    explicit Controller(const ControllerData& aData = {});
    virtual ~Controller() = default;
    virtual Tga::Vector2f GetDesiredVelocity(const Actor&) const { return {}; }
    virtual Tga::Vector2f GetSteeringForce(const Actor& aActor) const;
    virtual void Update(Actor&, float) {}
    virtual ControllerDebugInfo GetDebugInfo() const { return {}; }

    ContainmentDebugInfo GetContainmentDebugInfo(const Actor& aActor) const;

    const ControllerData& GetControllerData() const;
    void SetControllerData(const ControllerData& aData);
    void SetTraversalBounds(std::shared_ptr<const ITraversalBounds> aBounds);

protected:
    Tga::Vector2f GetContainmentForce(const Actor& aActor, bool& aIsOutsideBounds) const;
    Tga::Vector2f GetNearestValidPosition(const Actor& aActor, const Tga::Vector2f& aPosition) const;

private:
    ControllerData myData;
    std::shared_ptr<const ITraversalBounds> myTraversalBounds;
};

class TargetController : public Controller
{
public:
    explicit TargetController(const TargetControllerData& aData = {});
    void SetTargetPosition(const Tga::Vector2f& aPosition);
    void SetRandomTarget();
    void SetRandomTarget(const Actor& aActor);
    const Tga::Vector2f& GetTargetPosition() const;
    const TargetControllerData& GetTargetControllerData() const;
    void SetTargetControllerData(const TargetControllerData& aData);

protected:
    Tga::Vector2f myTargetPosition = {};
    void KeepTargetReachable(const Actor& aActor);

private:
    Tga::Vector2f GenerateRandomTarget();
    TargetControllerData myTargetData;
    std::mt19937 myRandomGenerator;
};
