#include "managers/camera_manager.hpp"
#include "core/components.hpp"
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"

auto gUp = glm::vec3(0.0f, 1.0f, 0.0f);

using namespace Apex;

void CameraManager::HandleKeyboard() const
{
    auto deltaTime = g_engine.GetDeltaTimeUnscaled();
    auto& camera = ECS::GetComponent<Component::Camera>(mCurrentCamera);
    auto& transform = ECS::GetComponent<Component::Transform>(mCurrentCamera);
    auto position = transform.m_position;
    auto translation = glm::vec3();

    const auto& input = g_engine.GetSystem<Input>();
    auto speed = mCameraSpeed * deltaTime;
    if (input->GetKeyboardKey(Input::EKeyboardKey::LeftShift))
    {
        speed *= 5;
    }

    if (input->GetKeyboardKey(Input::EKeyboardKey::W)) translation.z += speed;
    if (input->GetKeyboardKey(Input::EKeyboardKey::S)) translation.z -= speed;
    if (input->GetKeyboardKey(Input::EKeyboardKey::D)) translation.x += speed;
    if (input->GetKeyboardKey(Input::EKeyboardKey::A)) translation.x -= speed;
    if (input->GetKeyboardKey(Input::EKeyboardKey::Space)) translation.y += speed;
    if (input->GetKeyboardKey(Input::EKeyboardKey::LeftControl)) translation.y -= speed;
    position += translation.x * camera.mRight;
    position += translation.z * camera.mFront;
    position += translation.y * camera.mUp;

    transform.SetLocalPosition(position);
}

void CameraManager::HandleMouse(glm::vec2 delta) const
{
    delta.x *= mMouseSensitivity;
    delta.y *= mMouseSensitivity;

    auto& camera = ECS::GetComponent<Component::Camera>(mCurrentCamera);
    camera.mYaw += delta.x;
    camera.mPitch -= delta.y;

    camera.mPitch = glm::clamp(camera.mPitch, -89.f, 89.f);
}

void CameraManager::CreateCamera(const entt::entity& entity) { CreateCamera(entity, glm::vec3(0), 60.f, 0.1f, 1000.f); }

void CameraManager::CreateCamera(const Entity& entity, const glm::vec3 position, float fov, float nearClip,
                                 float farClip)
{
    ECS::GetComponent<Component::Transform>(entity).SetLocalPosition(position);
    ECS::AddComponent<Component::Camera>(entity, fov, nearClip, farClip);
    if (mCurrentCamera == entt::null) mCurrentCamera = entity;
}

glm::mat4 CameraManager::GetViewMatrix() const
{
    auto& transform = ECS::GetComponent<Component::Transform>(mCurrentCamera);
    auto& camera = ECS::GetComponent<Component::Camera>(mCurrentCamera);
    return lookAt(transform.m_position, transform.m_position + camera.mFront, camera.mUp);
}

glm::mat4 CameraManager::GetProjectionMatrix() const
{
    auto& camera = ECS::GetComponent<Component::Camera>(mCurrentCamera);
    const auto windowSize = g_engine.GetSystem<Device>()->GetWindowSize();
    return glm::perspective(glm::radians(camera.mFov), windowSize.x / windowSize.y, camera.mNear, camera.mFar);
}

glm::vec3 CameraManager::GetPosition() const
{
    auto& transform = ECS::GetComponent<Component::Transform>(mCurrentCamera);
    return transform.m_position;
}

void CameraManager::Init()
{
}

void CameraManager::Update(float)
{
    if (mCurrentCamera == entt::null)
    {
        if (mFailSafeCamera == entt::null)
        {
            mFailSafeCamera = ECS::CreateNamelessEntity();
            CreateCamera(mFailSafeCamera, glm::vec3(0, 0, -10));
        }
        else
        {
            mCurrentCamera = mFailSafeCamera;
        }
    }
    auto& [mFov, mNear, mFar, mFront, mUp, mRight, mYaw, mPitch] = ECS::GetComponent<Component::Camera>(mCurrentCamera);

    glm::vec3 direction;
    direction.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    direction.y = sin(glm::radians(mPitch));
    direction.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    mFront = normalize(direction);
    mRight = normalize(cross(mFront, gUp));
    mUp = normalize(cross(mRight, mFront));

    if (mCurrentCamera == mFailSafeCamera)
    {
        const auto input = g_engine.GetSystem<Input>();
        static auto previousMousePos = input->GetMousePosition();
        if (input->GetMouseButton(Input::EMouseButton::Right))
        {
            const auto currentMousePos = input->GetMousePosition();
            g_engine.GetSystem<CameraManager>()->HandleMouse(currentMousePos - previousMousePos);
        }
        previousMousePos = input->GetMousePosition();
        HandleKeyboard();
    }
}
