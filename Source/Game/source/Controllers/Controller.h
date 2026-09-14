#pragma once

#include <Vector2.hpp>
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
    CommonUtilities::Vector2f targetPosition = {};
    float slowingRadius = 0.f;
    bool hasSlowingRadius = false;
    float wanderCircleRadius = 0.f;
    float wanderCircleDistance = 0.f;
    CommonUtilities::Vector2f wanderCircleCenter = {};
    float wanderTargetAngle = 0.f;
};

struct ContainmentDebugInfo
{
    bool isEnabled = false;
    bool needsCorrection = false;
    CommonUtilities::Vector2f predictedPosition = {};
    CommonUtilities::Vector2f nearestValidPoint = {};
    float actorRadius = 0.f;
    float boundaryClearance = 0.f;
};

class Controller
{
public:
    explicit Controller(const ControllerData& aData = {});
    virtual ~Controller() = default;
    virtual CommonUtilities::Vector2f GetDesiredVelocity(const Actor&) const { return {}; }
    virtual CommonUtilities::Vector2f GetSteeringForce(const Actor& aActor) const;
    virtual float GetBehaviorWeight() const;
    virtual void Update(Actor&, float) {}
    virtual ControllerDebugInfo GetDebugInfo() const { return {}; }

    ContainmentDebugInfo GetContainmentDebugInfo(const Actor& aActor) const;

    const ControllerData& GetControllerData() const;
    void SetControllerData(const ControllerData& aData);
    void SetTraversalBounds(std::shared_ptr<const ITraversalBounds> aBounds);

protected:
    CommonUtilities::Vector2f GetContainmentForce(const Actor& aActor, bool& aIsOutsideBounds) const;
    CommonUtilities::Vector2f GetNearestValidPosition(const Actor& aActor, const CommonUtilities::Vector2f& aPosition) const;

private:
    ControllerData myData;
    std::shared_ptr<const ITraversalBounds> myTraversalBounds;
};

class TargetController : public Controller
{
public:
    explicit TargetController(const TargetControllerData& aData = {});
    void SetTargetPosition(const CommonUtilities::Vector2f& aPosition);
    void SetRandomTarget();
    void SetRandomTarget(const Actor& aActor);
    const CommonUtilities::Vector2f& GetTargetPosition() const;
    const TargetControllerData& GetTargetControllerData() const;
    void SetTargetControllerData(const TargetControllerData& aData);

protected:
    CommonUtilities::Vector2f myTargetPosition = {};
    void KeepTargetReachable(const Actor& aActor);

private:
    CommonUtilities::Vector2f GenerateRandomTarget();
    TargetControllerData myTargetData;
    std::mt19937 myRandomGenerator;
};
