#pragma once
#include "Actor.h"

#include <memory>
#include <vector>

class Controller;

struct BoidSettings
{
    float perceptionRadius = 100.f;
    float avoidanceRadius = 55.f;
    float alignmentWeight = 1.f;
    float cohesionWeight = 1.f;
    float separationWeight = 1.5f;
};

class Boid : public Actor
{
public:
    Boid();
    explicit Boid(std::unique_ptr<Controller> aController);
    ~Boid() override = default;

    void Update(float aDeltaTime) override;
    void SetFlockMembers(const std::vector<Boid*>& aFlockMembers);
    std::vector<const Boid*> GetFlockMembers() const;
    const BoidSettings& GetBoidSettings() const;
    void SetBoidSettings(const BoidSettings& aSettings);

protected:
    Tga::Vector2f GetAdditionalSteeringForce(float aDeltaTime) const override;

private:
    struct FlockMemberState
    {
        const Boid* boid = nullptr;
        Tga::Vector2f position = {};
        Tga::Vector2f velocity = {};
    };

    Tga::Vector2f SteerTowards(const Tga::Vector2f& aDirection) const;

    std::vector<FlockMemberState> myFlockMembers;
    BoidSettings mySettings;
};

