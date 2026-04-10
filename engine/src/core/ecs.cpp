
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/scene.hpp"
#include "managers/scene_manager.hpp"
#include "core/components.hpp"

using namespace Apex;

Ref<Registry> ECS::CreateRegistry()
{
    auto registry = std::make_shared<Registry>();
    for (auto [id, type] : entt::resolve())
    {
        using namespace entt::literals;
        if (auto registerConstruct = type.func("ConnectConstruct"_hs))
        {
            registerConstruct.invoke({}, registry);
        }
        if (auto registerUpdate = type.func("ConnectUpdate"_hs))
        {
            registerUpdate.invoke({}, registry);
        }
        if (auto registerDestroy = type.func("ConnectDestroy"_hs))
        {
            registerDestroy.invoke({}, registry);
        }
    }
    return registry;
}

Entity ECS::CreateEntity(const std::string& name)
{
    auto& registry = GetRegistry();
    const auto entity = registry.create();
    ECS::AddComponent<Component::Name>(entity, name);
    ECS::AddComponent<Component::Transform>(entity, entity, glm::vec3(), glm::vec3(), glm::vec3(1.f));
    ECS::AddComponent<Component::Hierarchy>(entity);
    return entity;
}

Entity ECS::CreateNamelessEntity()
{
    auto& registry = GetRegistry();
    const auto entity = registry.create();
    registry.emplace<Component::Transform>(entity, entity, glm::vec3(), glm::vec3(), glm::vec3(1.f));
    registry.emplace<Component::Hierarchy>(entity);
    return entity;
}

bool ECS::IsValid(const entt::entity entity) { return GetRegistry().valid(entity); }

bool ECS::GetParent(const Entity entity, Entity& parent)
{
    if (const auto* const parentComp = TryGetComponent<Component::Hierarchy>(entity))
    {
        if (!IsValid(parentComp->mParent)) return false;

        parent = parentComp->mParent;
        return true;
    }
    parent = entt::null;
    return false;
}

std::vector<Entity>& ECS::GetChildren(const Entity entity) { return GetComponent<Component::Hierarchy>(entity).mChildren; }

bool ECS::AddChild(const Entity parent, entt::entity child)
{
    GetComponent<Component::Hierarchy>(parent).mChildren.emplace_back(child);
    GetComponent<Component::Hierarchy>(child).mParent = parent;
    return true;
}

bool ECS::RemoveChild(const Entity parent, const Entity child)
{
    auto& children = GetComponent<Component::Hierarchy>(parent).mChildren;
    GetComponent<Component::Hierarchy>(child).mParent = entt::null;
    std::erase(children, child);
    return true;
}

entt::registry& ECS::GetRegistry() { return g_engine.GetSystem<SceneManager>()->GetCurrentScene()->GetRegistry(); }

void ECS::DestroyEntity(const Entity entity)
{
    Entity parent = entt::null;
    if (GetParent(entity, parent))
    {
        RemoveChild(parent, entity);
    }
    std::vector<entt::entity> children;
    for (const auto& child : GetChildren(entity))
    {
        if (HasComponent<Component::Renderable>(entity) || !HasComponent<Component::Name>(child))
        {
            g_engine.GetSystem<SceneManager>()->GetCurrentScene()->mEntitiesToRemove.emplace_back(child);
        }
        else
        {
            children.emplace_back(child);
        }
    }
    for (auto& child : children)
    {
        RemoveChild(entity, child);
    }
    g_engine.GetSystem<SceneManager>()->GetCurrentScene()->mEntitiesToRemove.emplace_back(entity);
}
