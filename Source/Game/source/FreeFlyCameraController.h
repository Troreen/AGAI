#pragma once
#include <tge/graphics/Camera.h>
#include <tge/input/InputManager.h>

class FreeFlyCameraController
{
public:
    FreeFlyCameraController();
    void Init(Tga::InputManager& anInputManager, Tga::Camera& aCamera);
    void Update(float aTimeDelta);
    void ResetMouseLookAnchor();
    void SetMoveSpeed(float aMoveSpeed);
    void SetLookSensitivity(float aLookSensitivity);
    float GetMoveSpeed() const;
    float GetLookSensitivity() const;
private:
    Tga::InputManager* myInputManager = nullptr;
    Tga::Camera* myCamera = nullptr;
    float myMoveSpeed = 500.f;
    float myLookSensitivity = 0.15f;
    Tga::Rotator myRotation{};
    bool myMouseLookActive = false;
    POINT myMouseAnchor{};
};
