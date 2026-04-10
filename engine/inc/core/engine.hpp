#pragma once
#include "system.hpp"

namespace Apex
{
    class EngineClass
    {
    public:
        void Init();
        void Shutdown();

        void BeginFrame();
        void Update();
        void EndFrame();

        template <typename System>
        Ref<System> AddSystem();
        template <typename System>
        [[nodiscard]] Ref<System> GetSystem();

        [[nodiscard]] float GetDeltaTime() const { return m_delta_time * m_time_scale; }
        [[nodiscard]] float GetDeltaTimeUnscaled() const { return m_delta_time; }
        [[nodiscard]] float GetFixedDeltaTime() const { return m_fixed_delta_time * m_time_scale; }

        bool m_paused = false;
        bool m_step = false;
        float m_time_scale = 1.0f;

    private:
        std::unordered_map<std::type_index, Ref<System>> m_systems;

        float m_delta_time = 0;
        float m_fixed_delta_time = 1 / 60.f;
    };

    template <typename System>
    Ref<System> EngineClass::GetSystem()
    {
        if (auto pointer = std::dynamic_pointer_cast<System>(m_systems[typeid(System)]))
        {
            return pointer;
        }
        return nullptr;
    }

    template <typename System>
    Ref<System> EngineClass::AddSystem()
    {
        m_systems[typeid(System)] = Apex::System::CreateSystem<System>();
        m_systems[typeid(System)]->Init();
        return std::static_pointer_cast<System>(m_systems[typeid(System)]);
    }

    extern EngineClass g_engine;

}  // namespace Apex