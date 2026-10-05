#include "Actor.h"
#include "../Controllers/Controller.h"

namespace
{
// Keep a movement or turning amount from going above its allowed maximum.
void LimitVector(CommonUtilities::Vector2f& aVector, const float aMaximumLength)
{
    const float length = aVector.Length();
    if (length > aMaximumLength && length > 0.f)
    {
        aVector = aVector.GetNormalized() * aMaximumLength;
    }
}
} // namespace

Actor::Actor()
{
}

Actor::~Actor() = default;

// --- Load the actor's picture and set its starting position ---
void Actor::SetTexture(const char* aTexturePath)
{
    mySharedData.texture = Tga::GraphicsEngine::GetInstance()->GetTextureManager().GetTexture(aTexturePath);
}

void Actor::Init(const CommonUtilities::Vector2f& aPosition, const char* aSpritePath)
{
    mySpritePath = aSpritePath;
    SetTexture(mySpritePath);
    myPosition = aPosition;
    mySpriteInstance.pivot = {0.5f, 0.5f};
    mySpriteInstance.position = myPosition.ToTga();
    mySpriteInstance.size = {50.f, 50.f};
    mySpriteInstance.color = Tga::Color(1, 1, 1, 1);
}

// --- Give the actor a brain ---
// The controller chooses what to do; this Actor owns it and handles movement.
void Actor::SetController(std::unique_ptr<Controller> aController)
{
    myControllers.clear();
    AddController(std::move(aController));
}

// --- Decide where to go, then move there ---
void Actor::Update(float aDeltaTime)
{
    CalculateSteering(aDeltaTime);
    UpdateMovement(aDeltaTime);
}

// Boids decide all steering before any boid moves, so neighbours share one frame.
void Actor::AddController(std::unique_ptr<Controller> aController)
{
    if (aController) myControllers.push_back(std::move(aController));
}

void Actor::CalculateSteering(float aDeltaTime, std::span<const Actor* const> aNeighbours)
{
    CommonUtilities::Vector2f totalSteering = {};
    for (const auto& controller : myControllers)
    {
        controller->Update(*this, aDeltaTime);
        totalSteering += controller->GetSteeringForce(*this, aNeighbours);
    }
    mySteeringForce = totalSteering;
}

bool Actor::NeedsNeighbours() const
{
    for (const auto& controller : myControllers)
        if (controller->NeedsNeighbours()) return true;
    return false;
}

const std::vector<std::unique_ptr<Controller>>& Actor::GetControllers() const
{
    return myControllers;
}

const CommonUtilities::Vector2f& Actor::GetPreviousSteeringForce() const
{
    return myPreviousSteeringForce;
}

// --- Turn the controller's steering into movement on screen ---
// Steering is a push that changes our speed and direction, not a new position.
void Actor::UpdateMovement(float aDeltaTime)
{
    LimitVector(mySteeringForce, myMaxForce);
    myPreviousSteeringForce = mySteeringForce;

    // The same push changes a lighter actor's speed more than a heavier actor's.
    myAcceleration = mySteeringForce / myMass;

    // Apply half the speed change before moving; the other half comes after moving.
    myVelocity += (myAcceleration * 0.5f) * aDeltaTime;
    LimitVector(myVelocity, myMaxSpeed);

    constexpr float standStillSpeed = 0.01f;
    if (myVelocity.LengthSqr() > (standStillSpeed * standStillSpeed))
    {
        myPosition += myVelocity * aDeltaTime;
    }

    myVelocity += (myAcceleration * 0.5f) * aDeltaTime;
    LimitVector(myVelocity, myMaxSpeed);

    // Make the sprite face its movement direction and follow the actor's position.
    myRotation = atan2f(myVelocity.y, myVelocity.x);
    mySpriteInstance.rotation = myRotation;
    mySpriteInstance.position = myPosition.ToTga();
    mySteeringForce = {};
}

// --- Read the actor's position, movement, and settings ---
const CommonUtilities::Vector2f& Actor::GetPosition() const
{
    return myPosition;
}
const CommonUtilities::Vector2f& Actor::GetVelocity() const
{
    return myVelocity;
}
const CommonUtilities::Vector2f& Actor::GetSteeringForce() const
{
    return mySteeringForce;
}

float Actor::GetMaxSpeed() const
{
    return myMaxSpeed;
}
float Actor::GetMaxForce() const
{
    return myMaxForce;
}
float Actor::GetMass() const
{
    return myMass;
}
float Actor::GetRadius() const
{
    return myRadius;
}

// --- Change the actor's movement settings or appearance ---
void Actor::SetMaxSpeed(float aValue)
{
    myMaxSpeed = aValue;
}
void Actor::SetMaxForce(float aValue)
{
    myMaxForce = aValue;
}
void Actor::SetMass(float aValue)
{
    myMass = aValue;
}
void Actor::SetRadius(float aValue)
{
    myRadius = aValue > 0.f ? aValue : 0.f;
}
void Actor::SetColor(const Tga::Color& aColor)
{
    mySpriteInstance.color = aColor;
}

// --- Move both the actor and its picture to a specific position ---
void Actor::SetPosition(const CommonUtilities::Vector2f& aPosition)
{
    myPosition = aPosition;
    mySpriteInstance.position = myPosition.ToTga();
}

// --- Stop immediately ---
// Clear both speed and the pending push so the actor does not keep drifting.
void Actor::Stop()
{
    myVelocity = {};
    myAcceleration = {};
    mySteeringForce = {};
}

// --- Let other parts of the game read the sprite and controller information ---
Tga::Sprite2DInstanceData Actor::GetSpriteInstanceData() const
{
    return mySpriteInstance;
}
Tga::SpriteSharedData Actor::GetSpriteSharedData() const
{
    return mySharedData;
}

const Controller* Actor::GetController() const
{
    return myControllers.empty() ? nullptr : myControllers.front().get();
}
Controller* Actor::GetController()
{
    return myControllers.empty() ? nullptr : myControllers.front().get();
}

// --- Draw this actor's picture ---
void Actor::Draw() const
{
    auto& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

    auto& spriteDrawer = graphicsEngine.GetSpriteDrawer();

    spriteDrawer.Draw(mySharedData, mySpriteInstance);
}
