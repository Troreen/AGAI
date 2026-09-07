#include "FreeFlyCameraController.h"
#include <algorithm>
#include <tge/application.h>

FreeFlyCameraController::FreeFlyCameraController() = default;

void FreeFlyCameraController::Init(Tga::InputManager& inputManager, Tga::Camera& camera)
{
    myInputManager = &inputManager;
    myCamera = &camera;
    // Matrix4x4 exposes directional vectors but not Euler-angle extraction.
    // GameWorld initializes the camera with this starting pitch.
    myRotation = Tga::Rotator(32.f, 0.f, 0.f);
    myMouseLookActive = false;
}

void FreeFlyCameraController::Update(float dt)
{
    if (!myInputManager || !myCamera) return;
    Tga::Matrix4x4f& transform = myCamera->GetTransform();
    Tga::Vector3f movement{};
    if (GetAsyncKeyState('W') & 0x8000) movement += transform.GetForward();
    if (GetAsyncKeyState('S') & 0x8000) movement -= transform.GetForward();
    if (GetAsyncKeyState('D') & 0x8000) movement += transform.GetRight();
    if (GetAsyncKeyState('A') & 0x8000) movement -= transform.GetRight();
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) movement += Tga::Vector3f::Up;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) movement -= Tga::Vector3f::Up;
    if (movement.LengthSqr() > 0.001f)
        transform.SetPosition(transform.GetPosition() + movement.GetNormalized() * myMoveSpeed * dt);

    const bool rightMouseDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    if (rightMouseDown)
    {
        if (!myMouseLookActive)
        {
            GetCursorPos(&myMouseAnchor);
            myInputManager->HideMouse();
            myInputManager->CaptureMouse();
            myMouseLookActive = true;
        }
        POINT cursorPosition{};
        GetCursorPos(&cursorPosition);
        const float mouseDeltaX = static_cast<float>(cursorPosition.x - myMouseAnchor.x);
        const float mouseDeltaY = static_cast<float>(cursorPosition.y - myMouseAnchor.y);
        myRotation.Y += mouseDeltaX * myLookSensitivity;
        myRotation.X = std::clamp(myRotation.X + mouseDeltaY * myLookSensitivity, -89.f, 89.f);
        SetCursorPos(myMouseAnchor.x, myMouseAnchor.y);
        transform.SetRotation(myRotation);
    }
    else if (myMouseLookActive)
    {
        myInputManager->ShowMouse();
        myInputManager->ReleaseMouse();
        myMouseLookActive = false;
    }
}

void FreeFlyCameraController::ResetMouseLookAnchor() { myMouseLookActive = false; }
void FreeFlyCameraController::SetMoveSpeed(float speed) { myMoveSpeed = (std::max)(1.f, speed); }
void FreeFlyCameraController::SetLookSensitivity(float sensitivity) { myLookSensitivity = (std::max)(0.001f, sensitivity); }
float FreeFlyCameraController::GetMoveSpeed() const { return myMoveSpeed; }
float FreeFlyCameraController::GetLookSensitivity() const { return myLookSensitivity; }
