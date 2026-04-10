#pragma once
#include "entt/entt.hpp"
#include "tools/log.hpp"

using Entity = entt::entity;
using Registry = entt::registry;

namespace Apex
{
    class Metadata;

    namespace Component
    {
        struct Hierarchy;
        struct Transform;
    }  // namespace Component

    class ECS
    {
    public:
        /// Creates a registry and connects all the necessary delegates
        /// @return A shared pointer to the created registry
        static Ref<Registry> CreateRegistry();

        /// Direct access to the entity registry for functions which don't have a direct wrapper
        /// @return The registry of the current scene
        static entt::registry& GetRegistry();

        /// Creates an entity and adds a name, hierarchy, and transform component.
        /// @param name The name of the entity to be created.
        /// @return The created entity.
        static Entity CreateEntity(const std::string& name);

        /// Creates an entity with a hierarchy and transform component.
        /// This is useful for entities that do not need to appear in the hierarchy panel,
        /// such as children for transforms in particle systems.
        /// @return The created entity.
        [[nodiscard]] static Entity CreateNamelessEntity();

        /// Destroys the entity completely along with its children (named entities are spared).
        /// Removed as a child from the parent
        /// @param entity Entity to destroy
        static void DestroyEntity(Entity entity);

        /// Checks if the entity is valid or not
        /// @param entity The entity to check
        /// @return True if the entity is still valid; otherwise, False
        [[nodiscard]] static bool IsValid(Entity entity);

        /// Gets the parent of a given entity, if found
        /// @param entity The entity whose parent needs to be accessed
        /// @param parent Returns a valid parent if found, otherwise returns entt::null
        /// @return true if the entity has a parent, false if not
        [[nodiscard]] static bool GetParent(Entity entity, Entity& parent);

        /// Gets all direct children on a given entity
        /// @param entity The entity whose children need to be accessed
        /// @return a vector of all direct children
        [[nodiscard]] static std::vector<Entity>& GetChildren(Entity entity);

        /// Adds a child entity to the specified parent entity.
        /// @param parent The entity to which the child will be added.
        /// @param child The child entity to add to the parent.
        /// @return True if the child was successfully added; otherwise, false.
        static bool AddChild(Entity parent, Entity child);

        /// Removes a child from the given parent
        /// @param parent The entity from which the child will be removed
        /// @param child The child to remove from the parent
        /// @return True if the child was successfully added; otherwise, false.
        static bool RemoveChild(Entity parent, Entity child);

        /// Adds a component of the specified type to the given entity with provided arguments.
        /// @tparam Component The type of component to add to the entity.
        /// @tparam Args The types of arguments to forward to the component's constructor.
        /// @param entity The entity to which the component will be added.
        /// @param args Arguments to forward to the component's constructor.
        /// @return A reference to the newly added component.
        template <class Component, class... Args>
        static Component& AddComponent(Entity entity, Args&&... args);

        /// Checks if an entity has the specified component(s)
        /// @tparam Component The type of component to check for
        /// @param entity The entity to check on
        /// @return True if the entity has the given component; otherwise; false
        template <typename... Component>
        static bool HasComponent(Entity entity);

        /// Gets the given component on an entity
        /// @tparam Component The component(s) to check for
        /// @param entity The entity from which to get the component
        /// @return The actual component(s) on the entity
        template <typename... Component>
        static decltype(auto) GetComponent(Entity entity);

        /// Attempts to get a component of the specified type from the given entity.
        /// @tparam Component The type of component to get.
        /// @param entity The entity to get the component from
        /// @return A pointer to the component if it exists on the entity; otherwise, returns nullptr.
        template <typename Component>
        static Component* TryGetComponent(Entity entity);

        /// Removes a component from a given entity
        /// @tparam Component The type of component to remove
        /// @param entity The entity to remove the component from
        template <typename... Component>
        static void RemoveComponent(Entity entity);

    private:
        friend class Metadata;

        /// Adds a delegate function which gets invoked when a component is added to an entity
        /// @tparam Component The type of component to connect the delegate for.
        /// @param registry A shared pointer to the registry in which this function is valid.
        template <typename Component>
        static void ConnectConstruct(const Ref<entt::registry>& registry);

        /// Adds a delegate function that gets invoked when a component is updated for an entity.
        /// @tparam Component The type of component to connect the delegate for.
        /// @param registry A shared pointer to the registry in which this function is valid.
        template <class Component>
        static void ConnectUpdate(const Ref<entt::registry>& registry);

        /// Adds a delegate function that gets invoked when a component is destroyed for an entity.
        /// @tparam Component The type of component to connect the delegate for.
        /// @param registry A shared pointer to the registry in which this function is valid.
        template <typename Component>
        static void ConnectDestroy(const Ref<entt::registry>& registry);
    };

    template <typename Component, typename... Args>
    Component& ECS::AddComponent(Entity entity, Args&&... args)
    {
        return GetRegistry().emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template <typename... Component>
    bool ECS::HasComponent(const Entity entity)
    {
        return GetRegistry().all_of<Component...>(entity);
    }

    template <typename... Component>
    void ECS::RemoveComponent(Entity entity)
    {
        GetRegistry().remove<Component...>(entity);
    }

    template <typename Component>
    void ECS::ConnectConstruct(const Ref<entt::registry>& registry)
    {
        registry->on_construct<Component>()
            .template connect<[&](entt::registry&, entt::entity entity) { Component::Construct(entity); }>();
    }

    template <typename Component>
    void ECS::ConnectDestroy(const Ref<entt::registry>& registry)
    {
        registry->on_destroy<Component>()
            .template connect<[&](entt::registry&, entt::entity entity) { Component::Destroy(entity); }>();
    }

    template <class Component>
    void ECS::ConnectUpdate(const Ref<entt::registry>& registry)
    {
        registry->on_update<Component>()
            .template connect<[](entt::registry&, entt::entity entity) { Component::Update(entity); }>();
    }

    template <typename... Component>
    decltype(auto) ECS::GetComponent(const Entity entity)
    {
        return GetRegistry().get<Component...>(entity);
    }

    template <typename Component>
    Component* ECS::TryGetComponent(Entity entity)
    {
        return GetRegistry().try_get<Component>(entity);
    }
}  // namespace Apex
