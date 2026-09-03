#pragma once

#include <tge/sprite/sprite.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/application.h>
#include <memory>
class Controller;


class Actor
{
public:
    Actor();
    virtual ~Actor();

    void Init(const Tga::Vector2f& aPosition, const char* aSpritePath);
    void Update(float aDeltaTime);
    void UpdateMovement(float aDeltaTime);
    void SetController(std::unique_ptr<Controller> aController);
    void Draw() const;
    const Tga::Vector2f& GetPosition() const;
    const Tga::Vector2f& GetVelocity() const;
	const Tga::Vector2f& GetSteeringForce() const;
	const Tga::Vector2f& GetPreviousSteeringForce() const;

    float GetMaxSpeed() const;
    float GetMaxForce() const;
    float GetMass() const;
    float GetRadius() const;
    void SetMaxSpeed(float aValue);
    void SetMaxForce(float aValue);
    void SetMass(float aValue);
    void SetRadius(float aValue);
    

	Tga::Sprite2DInstanceData GetSpriteInstanceData() const;
	Tga::SpriteSharedData GetSpriteSharedData() const;
    const Controller* GetController() const;
    Controller* GetController();

    void AddSteeringForce(const Tga::Vector2f& aForce);

protected:
    void SetTexture(const char* aTexturePath);
    
    Tga::Sprite2DInstanceData mySpriteInstance = {};
    Tga::SpriteSharedData mySharedData = {};
	const char* mySpritePath = nullptr;


private:

    std::unique_ptr<Controller> myController;
    Tga::Vector2f myPosition;
    Tga::Vector2f myVelocity;
    Tga::Vector2f myAcceleration;
    Tga::Vector2f mySteeringForce;
    Tga::Vector2f myPreviousSteeringForce;

    // Maximum steering force accepted each frame; caps turning/acceleration.
    float myMaxForce = 500.f;
    // Higher mass makes the same steering force produce less acceleration.
    float myMass = 1.f;
    // Maximum movement speed used by all desired-velocity controllers.
    float myMaxSpeed = 250.f;
    float myRotation = 0.f;
    // Collision/occupancy radius used to inset traversal bounds.
    float myRadius = 25.f;

};
