#include "Actor.h"
#include "Controller.h"

namespace
{
	void LimitVector(Tga::Vector2f& aVector, const float aMaximumLength)
	{
		const float length = aVector.Length();
		if (length > aMaximumLength && length > 0.f)
			aVector = aVector.GetNormalized() * aMaximumLength;
	}
}


Actor::Actor()
{
}

Actor::~Actor()
{
}

void Actor::SetTexture(const char* aTexturePath)
{
	mySharedData.texture = 
		Tga::GraphicsEngine::GetInstance()
			->GetTextureManager()
			.GetTexture(aTexturePath);
}

void Actor::Init(const Tga::Vector2f& aPosition, const char* aSpritePath)
{
	mySpritePath = aSpritePath;
	SetTexture(mySpritePath);
	myPosition = aPosition;
    mySpriteInstance.pivot = { 0.5f, 0.5f };
    mySpriteInstance.position = myPosition;
    mySpriteInstance.size = { 50.f, 50.f };
    mySpriteInstance.color = Tga::Color(1, 1, 1, 1);

}

void Actor::SetController(std::unique_ptr<Controller> aController)
{
    myController = std::move(aController);
}

void Actor::Update(float aDeltaTime)
{
    if (myController)
    {
        myController->Update(*this, aDeltaTime);
        mySteeringForce = myController->GetSteeringForce(*this);
    }
    UpdateMovement(aDeltaTime);
    mySpriteInstance.position = myPosition;
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
	mySteeringForce.Set(0.f, 0.f);
}

const Tga::Vector2f& Actor::GetPosition() const { return myPosition; }
const Tga::Vector2f& Actor::GetVelocity() const { return myVelocity; }
const Tga::Vector2f& Actor::GetSteeringForce() const { return mySteeringForce; }
const Tga::Vector2f& Actor::GetPreviousSteeringForce() const { return myPreviousSteeringForce; }

float Actor::GetMaxSpeed() const { return myMaxSpeed; }
float Actor::GetMaxForce() const { return myMaxForce; }
float Actor::GetMass() const { return myMass; }
float Actor::GetRadius() const { return myRadius; }
void Actor::SetMaxSpeed(float aValue) { myMaxSpeed = aValue; }
void Actor::SetMaxForce(float aValue) { myMaxForce = aValue; }
void Actor::SetMass(float aValue) { myMass = aValue; }
void Actor::SetRadius(float aValue) { myRadius = aValue > 0.f ? aValue : 0.f; }
Tga::Sprite2DInstanceData Actor::GetSpriteInstanceData() const { return mySpriteInstance; }
Tga::SpriteSharedData Actor::GetSpriteSharedData() const { return mySharedData; }

const Controller* Actor::GetController() const { return myController.get(); }
Controller* Actor::GetController() { return myController.get(); }


void Actor::AddSteeringForce(const Tga::Vector2f& aForce) { mySteeringForce += aForce; }

void Actor::Draw() const
{
    auto& graphicsEngine = *Tga::GraphicsEngine::GetInstance();

	auto& spriteDrawer = graphicsEngine.GetSpriteDrawer();
	
	spriteDrawer.Draw(mySharedData, mySpriteInstance);
}
