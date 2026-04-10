#pragma once
#include "core/ecs.hpp"
#include "core/system.hpp"
#include "entt/entt.hpp"

namespace Apex
{
    namespace Component
    {
        struct Camera
        {
            float mFov = 60.f;
            float mNear = 0.1f;
            float mFar = 100.f;
            glm::vec3 mFront = {0.0f, 0.0f, -1.0f};
            glm::vec3 mUp = {0.0f, 1.0f, 0.0f};
            glm::vec3 mRight = {1.0f, 0.0f, 0.0f};
            float mYaw = 90.0f;
            float mPitch = 0.0f;
            static void Inspector(Entity entity);
        };
    } // namespace Component

    class CameraManager final : public System
    {
    public:
        void HandleKeyboard() const;
        void HandleMouse(glm::vec2 delta) const;
        void CreateCamera(const Entity& entity);
        void CreateCamera(const Entity& entity,
                          glm::vec3 position,
                          float fov = 60.f,
                          float nearClip = 0.1f,
                          float farClip = 1000.f);
        void SetCurrentCamera(const Entity camera) { mCurrentCamera = camera; }
        Entity GetCurrentCamera() const { return mCurrentCamera; }
        
        [[nodiscard]] glm::mat4 GetViewMatrix() const;
        [[nodiscard]] glm::mat4 GetProjectionMatrix() const;
        [[nodiscard]] glm::vec3 GetPosition() const;

    private:
        friend class System;
        friend class SceneManager;

        void Init() override;
        void Update(float deltaTime) override;

        Entity mCurrentCamera = entt::null;
        Entity mFailSafeCamera = entt::null;
        float mCameraSpeed = 5.f;
        float mMouseSensitivity = 0.1f;
    };
} // namespace bee
