#pragma once
#include "memory"
#include "core/system.hpp"

namespace Apex
{

    class Scene;
    class EngineClass;

    class SceneManager : public System
    {
    public:
        [[nodiscard]] Ref<Scene> GetCurrentScene() const { return m_current_scene; }
        void SetActiveScene(const Ref<Scene>& scene);

    private:
        friend class EngineClass;
        friend class System;
        void Update(float deltaTime) override;
        void FixedUpdate(float deltaTime) override;
        void EndFrame() override;

        Ref<Scene> m_current_scene;
        Ref<Scene> m_next_scene;
    };

}  // namespace Apex