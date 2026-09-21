#include "Actor.h"
#include "Controllers/Controller.h"

namespace
{
	void LimitVector(CommonUtilities::Vector2f& aVector, const float aMaximumLength)
	{
		const float length = aVector.Length();
		if (length > aMaximumLength && length > 0.f)
			aVector = aVector.GetNormalized() * aMaximumLength;
	}
}


Actor::Actor()
{
}

Actor::~Actor() = default;

void Actor::SetTexture(const char* aTexturePath)
{
	mySharedData.texture = 
		Tga::GraphicsEngine::GetInstance()
			->GetTextureManager()
			.GetTexture(aTexturePath);
}

void Actor::Init(const CommonUtilities::Vector2f& aPosition, const char* aSpritePath)
{
	mySpritePath = aSpritePath;
	SetTexture(mySpritePath);
	myPosition = aPosition;
    mySpriteInstance.pivot = { 0.5f, 0.5f };
    mySpriteInstance.position = myPosition.ToTga();
    mySpriteInstance.size = { 50.f, 50.f };
    mySpriteInstance.color = Tga::Color(1, 1, 1, 1);

}

void Actor::SetController(std::unique_ptr<Controller> aController)
{
    myControllers.clear();
    AddController(std::move(aController));
}

void Actor::AddController(std::unique_ptr<Controller> aController)
{
    if (aController)
    {
        myControllers.push_back(std::move(aController));
    }
}

bool Actor::NeedsNeighbours() const
{
    for (const auto& controller : myControllers)
    {
        if (controller->NeedsNeighbours())
            return true;
    }
    return false;
}

void Actor::Update(float aDeltaTime)
{
    CalculateSteering(aDeltaTime);
    UpdateMovement(aDeltaTime);
}

void Actor::CalculateSteering(float aDeltaTime, std::span<const Actor* const> aNeighbours)
{
    CommonUtilities::Vector2f totalSteering = {};
    for (const std::unique_ptr<Controller>& controller : myControllers)
    {
        controller->Update(*this, aDeltaTime);
        totalSteering += controller->GetSteeringForce(*this, aNeighbours);
    }
    mySteeringForce = totalSteering;
}

void Actor::UpdateMovement(float aDeltaTime)
{
	LimitVector(mySteeringForce, myMaxForce);
	myPreviousSteeringForce = mySteeringForce;

	myAcceleration = mySteeringForce / myMass;

	myVelocity += (myAcceleration * 0.5f) * aDeltaTime;
	LimitVector(myVelocity, myMaxSpeed);

	constexpr float standStillSpeed = 0.01f;
	if (myVelocity.LengthSqr() > (standStillSpeed * standStillSpeed))
	{
		myPosition += myVelocity * aDeltaTime;
	}
	
	myVelocity += (myAcceleration * 0.5f) * aDeltaTime;
	LimitVector(myVelocity, myMaxSpeed);

	myRotation = atan2f(myVelocity.y, myVelocity.x);
	mySpriteInstance.rotation = myRotation;
	mySpriteInstance.position = myPosition.ToTga();
	mySteeringForce = {};
}

const CommonUtilities::Vector2f& Actor::GetPosition() const { return myPosition; }
const CommonUtilities::Vector2f& Actor::GetVelocity() const { return myVelocity; }
const CommonUtilities::Vector2f& Actor::GetSteeringForce() const { return mySteeringForce; }
const CommonUtilities::Vector2f& Actor::GetPreviousSteeringForce() const { return myPreviousSteeringForce; }

float Actor::GetMaxSpeed() const { return myMaxSpeed; }
float Actor::GetMaxForce() const { return myMaxForce; }
float Actor::GetMass() const { return myMass; }
float Actor::GetRadius() const { return myRadius; }

void Actor::SetMaxSpeed(float aValue) { myMaxSpeed = aValue; }
void Actor::SetMaxForce(float aValue) { myMaxForce = aValue; }
void Actor::SetMass(float aValue) { myMass = aValue; }
void Actor::SetRadius(float aValue) { myRadius = aValue > 0.f ? aValue : 0.f; }
void Actor::SetColor(const Tga::Color& aColor) { mySpriteInstance.color = aColor; }

Tga::Sprite2DInstanceData Actor::GetSpriteInstanceData() const { return mySpriteInstance; }
Tga::SpriteSharedData Actor::GetSpriteSharedData() const { return mySharedData; }

const Controller* Actor::GetController() const { return myControllers.empty() ? nullptr : myControllers.front().get(); }
Controller* Actor::GetController() { return myControllers.empty() ? nullptr : myControllers.front().get(); }
const std::vector<std::unique_ptr<Controller>>& Actor::GetControllers() const { return myControllers; }


void Actor::AddSteeringForce(const CommonUtilities::Vector2f& aForce) { mySteeringForce += aForce; }

void Actor::Draw() const
{
    auto& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

	auto& spriteDrawer = graphicsEngine.GetSpriteDrawer();
	
	spriteDrawer.Draw(mySharedData, mySpriteInstance);
}
