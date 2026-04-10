#include "core/engine.hpp"
#include "glm/common.hpp"
#include "core/device.hpp"
#include "core/input.hpp"
#include "core/renderer.hpp"
#include "managers/camera_manager.hpp"
#include "managers/scene_manager.hpp"

using namespace Apex;

EngineClass Apex::g_engine;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void EngineClass::Init()
{
    AddSystem<Device>();
    AddSystem<Input>();
    AddSystem<Renderer>();
    AddSystem<ResourceManager>();
    AddSystem<SceneManager>();
    AddSystem<CameraManager>();
}

void EngineClass::Shutdown()
{
    for (const auto& system : std::views::values(m_systems))
    {
        system->Shutdown();
    }
}

void EngineClass::BeginFrame()
{
    for (const auto& system : std::views::values(m_systems))
    {
        system->BeginFrame();
    }
}

void EngineClass::Update()
{
    static auto previousTime = std::chrono::high_resolution_clock::now();
    static auto lastUpdate = 0.f;
    m_delta_time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - previousTime).count();
    m_delta_time = glm::clamp(m_delta_time, 0.0f, 0.5f);
    previousTime = std::chrono::high_resolution_clock::now();
    lastUpdate += m_delta_time;

    if (m_step) m_paused = false;
    while (lastUpdate >= m_fixed_delta_time)
    {
        lastUpdate -= m_fixed_delta_time;
        if (!m_paused)
        {
            for (const auto& system : std::views::values(m_systems))
            {
                system->FixedUpdate(GetFixedDeltaTime());
            }
        }
    }

    if (!m_paused)
    {
        for (const auto& system : std::views::values(m_systems))
        {
            system->Update(GetDeltaTime());
        }
    }
    else
    {
        for (auto& system : std::views::values(m_systems))
        {
            if (system->bCanBePaused) return;
            system->Update(GetDeltaTime());
        }
    }

    if (m_step)
    {
        m_paused = true;
        m_step = false;
    }
}

void EngineClass::EndFrame()
{
    for (const auto& system : std::views::values(m_systems))
    {
        system->EndFrame();
    }
}