#include "Boid3D.h"
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/ModelDrawer.h>
#include <algorithm>
#include <cmath>

Boid3D::Boid3D(const Tga::ModelInstance& aModel, const Tga::Vector3f& aPosition, unsigned int aSeed)
    : myPosition(aPosition), myModel(aModel), myRandom(aSeed)
{
    myModel.GetTransform().SetPosition(myPosition);
    myModel.GetTransform().Scale({ 18.f, 18.f, 18.f });
}

Tga::Vector3f Boid3D::Steer(const Tga::Vector3f& direction) const
{
    if (direction.LengthSqr() <= 0.0001f) return {};
    return direction.GetNormalized() * 650.f - myVelocity;
}

void Boid3D::Update(float dt, const std::vector<Boid3D*>& flock,
    const std::vector<SphericalObstacle<Tga::Vector3f>>& obstacles,
    const RectTraversalBounds3D& bounds)
{
    Tga::Vector3f alignment{}, centre{}, separation{};
    int count = 0;
    for (const Boid3D* other : flock)
    {
        if (other == this) continue;
        const Tga::Vector3f offset = other->myPosition - myPosition;
        const float distance = offset.Length();
        if (distance <= 0.001f || distance > mySettings.perceptionRadius) continue;
        alignment += other->myVelocity;
        centre += other->myPosition;
        if (distance < mySettings.separationRadius)
            separation -= offset.GetNormalized() * ((mySettings.separationRadius - distance) / mySettings.separationRadius);
        ++count;
    }

    Tga::Vector3f force{};
    if (count > 0)
    {
        alignment /= static_cast<float>(count);
        centre /= static_cast<float>(count);
        force += Steer(alignment) * mySettings.alignmentWeight;
        force += Steer(centre - myPosition) * mySettings.cohesionWeight;
        force += Steer(separation) * mySettings.separationWeight;
    }

    std::uniform_real_distribution<float> jitter(-1.f, 1.f);
    myWanderAngle += jitter(myRandom) * dt * 1.2f;
    Tga::Vector3f forward = myVelocity.LengthSqr() > 0.001f ? myVelocity.GetNormalized() : Tga::Vector3f::Forward;
    Tga::Vector3f wander = (forward + Tga::Vector3f(std::cos(myWanderAngle), jitter(myRandom) * 0.3f, std::sin(myWanderAngle)) * 0.35f).GetNormalized();
    force += Steer(wander) * mySettings.wanderWeight;

    const Tga::Vector3f safeDirection = ObstacleAvoidance::Avoid3D(myPosition, forward,
        mySettings.avoidanceLookAhead, mySettings.avoidanceClearance, obstacles);
    if ((safeDirection - forward).LengthSqr() > 0.0001f)
        force += Steer(safeDirection) * mySettings.obstacleWeight;

    const Tga::Vector3f predicted = myPosition + myVelocity * 0.8f;
    const Tga::Vector3f recovery = bounds.GetRecoveryDirection(myPosition, predicted, mySettings.avoidanceClearance);
    force += Steer(recovery) * 3.f;

    const float maxForce = 1200.f;
    if (force.Length() > maxForce) force = force.GetNormalized() * maxForce;
    Tga::Vector3f acceleration = force;
    myVelocity += acceleration * dt;
    if (myVelocity.Length() > 650.f) myVelocity = myVelocity.GetNormalized() * 650.f;
    myPosition += myVelocity * dt;
    myPosition = bounds.GetNearestValidPoint(myPosition, mySettings.avoidanceClearance);

    myModel.GetTransform().SetPosition(myPosition);
    if (myVelocity.LengthSqr() > 0.01f)
    {
        // cone.fbx points along local +Y. Build an orthonormal basis whose
        // local Y axis follows the boid velocity.
        const Tga::Vector3f localY = myVelocity.GetNormalized();
        const Tga::Vector3f reference = std::abs(localY.Dot(Tga::Vector3f::Up)) > 0.95f
            ? Tga::Vector3f::Forward : Tga::Vector3f::Up;
        const Tga::Vector3f localX = localY.Cross(reference).GetNormalized();
        const Tga::Vector3f localZ = localX.Cross(localY).GetNormalized();
        const Tga::Quaternionf rotation = Tga::Quaternionf::CreateFromOrthonormalBasisVectors(localX, localY, localZ);
        Tga::Matrix4x4f modelTransform = Tga::Matrix4x4f::CreateFromRotation(rotation);
        modelTransform.SetPosition(myPosition);
        modelTransform.Scale({ 55.f, 55.f, 55.f });
        myModel.SetTransform(modelTransform);
    }
}

void Boid3D::Draw() const
{
    if (myModel.IsValid())
        Tga::GraphicsEngine::GetInstance()->GetModelDrawer().Draw(myModel);
}
