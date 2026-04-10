#include "managers/scene_manager.hpp"
#include "core/ecs.hpp"
#include "core/scene.hpp"

using namespace Apex;

void SceneManager::EndFrame()
{
    for (const auto& entity : m_current_scene->mEntitiesToRemove)
    {
        if (ECS::IsValid(entity))
        {
            ECS::GetRegistry().destroy(entity);
        }
    }
    m_current_scene->mEntitiesToRemove.clear();
    if (m_next_scene)
    {
        const auto view = ECS::GetRegistry().view<Entity>();
        view.each(
            [&](const Entity entity)
            {
                if (ECS::IsValid(entity))
                {
                    ECS::GetRegistry().destroy(entity);
                }
            });
        m_current_scene = std::move(m_next_scene);
        m_current_scene->Init();
        m_next_scene = nullptr;
    }
}

void SceneManager::SetActiveScene(const Ref<Scene>& scene)
{
    if (!m_current_scene)
    {
        m_current_scene = std::move(scene);
        m_current_scene->Init();
    }
    else
    {
        if (m_current_scene != scene) m_next_scene = std::move(scene);
    }
}

void SceneManager::Update(const float deltaTime) { m_current_scene->Update(deltaTime); }

void SceneManager::FixedUpdate(const float deltaTime) { m_current_scene->FixedUpdate(deltaTime); }