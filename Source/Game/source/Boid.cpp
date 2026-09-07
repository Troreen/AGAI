#include "Boid.h"
#include "Controller.h"

Boid::Boid() = default;

Boid::Boid(std::unique_ptr<Controller> aController)
{
    SetController(std::move(aController));
}

void Boid::Update(float aDeltaTime)
{
    // Keep the common controller, flocking, movement, and sprite update path.
    Actor::Update(aDeltaTime);
}

void Boid::SetFlockMembers(const std::vector<Boid*>& aFlockMembers)
{
    myFlockMembers.clear();
    myFlockMembers.reserve(aFlockMembers.size());
    for (const Boid* boid : aFlockMembers)
    {
        if (boid != nullptr)
            myFlockMembers.push_back({ boid, boid->GetPosition(), boid->GetVelocity() });
    }
}

std::vector<const Boid*> Boid::GetFlockMembers() const
{
    std::vector<const Boid*> flockMembers;
    for (const FlockMemberState& member : myFlockMembers)
    {
        if (member.boid == nullptr || member.boid == this)
            continue;

        if ((member.position - GetPosition()).Length() <= mySettings.perceptionRadius)
            flockMembers.push_back(member.boid);
    }
    return flockMembers;
}

const BoidSettings& Boid::GetBoidSettings() const
{
    return mySettings;
}

void Boid::SetBoidSettings(const BoidSettings& aSettings)
{
    mySettings = aSettings;
}

Tga::Vector2f Boid::SteerTowards(const Tga::Vector2f& aDirection) const
{
    if (aDirection.LengthSqr() <= 0.0001f)
        return {};

    return aDirection.GetNormalized() * GetMaxSpeed() - GetVelocity();
}

Tga::Vector2f Boid::GetAdditionalSteeringForce(float) const
{
    Tga::Vector2f averageHeading = {};
    Tga::Vector2f flockCentre = {};
    Tga::Vector2f separationHeading = {};
    int perceivedFlockmates = 0;

    for (const FlockMemberState& other : myFlockMembers)
    {
        if (other.boid == this)
            continue;

        const Tga::Vector2f offset = other.position - GetPosition();
        const float distance = offset.Length();
        if (distance <= 0.0001f || distance > mySettings.perceptionRadius)
            continue;

        ++perceivedFlockmates;
        averageHeading += other.velocity;
        flockCentre += other.position;

        if (distance < mySettings.avoidanceRadius)
        {
            separationHeading -= offset.GetNormalized()
                * ((mySettings.avoidanceRadius - distance) / mySettings.avoidanceRadius);
        }
    }

    if (perceivedFlockmates == 0)
        return {};

    const float neighbourCount = static_cast<float>(perceivedFlockmates);
    averageHeading /= neighbourCount;
    flockCentre /= neighbourCount;

    Tga::Vector2f force = {};
    force += SteerTowards(averageHeading) * mySettings.alignmentWeight;
    force += SteerTowards(flockCentre - GetPosition()) * mySettings.cohesionWeight;
    force += SteerTowards(separationHeading) * mySettings.separationWeight;
    return force;
}
