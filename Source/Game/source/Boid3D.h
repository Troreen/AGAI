#pragma once

#include "ObstacleAvoidance.h"
#include "TraversalBounds.h"
#include <tge/model/ModelInstance.h>
#include <memory>
#include <random>

struct Boid3DSettings
{
    float perceptionRadius = 500.f;
    float separationRadius = 180.f;
    float avoidanceLookAhead = 700.f;
    float avoidanceClearance = 45.f;
    float alignmentWeight = 1.f;
    float cohesionWeight = 0.8f;
    float separationWeight = 1.8f;
    float obstacleWeight = 3.f;
    float wanderWeight = 0.35f;
};

class Boid3D
{
public:
    Boid3D(const Tga::ModelInstance& aModel, const Tga::Vector3f& aPosition, unsigned int aSeed);
    void Update(float aDeltaTime, const std::vector<Boid3D*>& aFlock,
        const std::vector<SphericalObstacle<Tga::Vector3f>>& aObstacles,
        const RectTraversalBounds3D& aBounds);
    void Draw() const;
    void SetSettings(const Boid3DSettings& aSettings) { mySettings = aSettings; }
    const Boid3DSettings& GetSettings() const { return mySettings; }
    const Tga::Vector3f& GetPosition() const { return myPosition; }
    const Tga::Vector3f& GetVelocity() const { return myVelocity; }
    Tga::ModelInstance& GetModel() { return myModel; }

private:
    Tga::Vector3f Steer(const Tga::Vector3f& direction) const;
    Tga::Vector3f myPosition{};
    Tga::Vector3f myVelocity{};
    Tga::ModelInstance myModel;
    Boid3DSettings mySettings;
    std::mt19937 myRandom;
    float myWanderAngle = 0.f;
};
