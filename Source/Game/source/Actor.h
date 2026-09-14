#pragma once

#include <tge/sprite/sprite.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/application.h>
#include <Vector2.hpp>
#include <memory>
#include <vector>
class Controller;


class Actor
{
public:
    Actor();
    virtual ~Actor() = default;

    virtual void Init(const CommonUtilities::Vector2f& aPosition, const char* aSpritePath);
    void Update(float aDeltaTime);
    void UpdateMovement(float aDeltaTime);
    void SetController(std::unique_ptr<Controller> aController);
    void AddController(std::unique_ptr<Controller> aController);
    void SetNeighbours(std::vector<const Actor*> aNeighbours);
    virtual void Draw() const;
    const CommonUtilities::Vector2f& GetPosition() const;
    const CommonUtilities::Vector2f& GetVelocity() const;
    const std::vector<const Actor*>& GetNeighbours() const;
	const CommonUtilities::Vector2f& GetSteeringForce() const;
	const CommonUtilities::Vector2f& GetPreviousSteeringForce() const;

    float GetMaxSpeed() const;
    float GetMaxForce() const;
    float GetMass() const;
    float GetRadius() const;
    void SetMaxSpeed(float aValue);
    void SetMaxForce(float aValue);
    void SetMass(float aValue);
    void SetRadius(float aValue);
    void SetColor(const Tga::Color& aColor);
    

	Tga::Sprite2DInstanceData GetSpriteInstanceData() const;
	Tga::SpriteSharedData GetSpriteSharedData() const;
    const Controller* GetController() const;
    Controller* GetController();
    const std::vector<std::unique_ptr<Controller>>& GetControllers() const;

    void AddSteeringForce(const CommonUtilities::Vector2f& aForce);

protected:
    void SetTexture(const char* aTexturePath);
    Tga::Sprite2DInstanceData mySpriteInstance = {};
    Tga::SpriteSharedData mySharedData = {};
	const char* mySpritePath = nullptr;


private:

    std::vector<std::unique_ptr<Controller>> myControllers;
    std::vector<const Actor*> myNeighbours; // create list on stack in gameworld and preallocate memory for it so creaete/destor/pushback is cheap. you can do it thrugh a function in gameworld and none neds to own the list essentially. 
    CommonUtilities::Vector2f myPosition;
    CommonUtilities::Vector2f myVelocity;
    CommonUtilities::Vector2f myAcceleration;
    CommonUtilities::Vector2f mySteeringForce;
    CommonUtilities::Vector2f myPreviousSteeringForce;

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
