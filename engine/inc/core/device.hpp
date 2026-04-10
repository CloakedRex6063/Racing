#pragma once
#include "core/system.hpp"
#include "GLFW/glfw3.h"

namespace Apex
{
    class Device final : public System
    {
    public:
        Device();
        ~Device() override;

        void Init() override;
        void Update(float deltaTime) override;
        void Shutdown() override;

        [[nodiscard]] glm::vec2 GetWindowSize() const;
        [[nodiscard]] bool ShouldClose() const;
        [[nodiscard]] void* GetWindow() const;
        [[nodiscard]] void* GetNativeWindow() const;
        void LockMouse(bool toggle) const;

    private:
        GLFWwindow* m_window;
    };

}  // namespace Apex