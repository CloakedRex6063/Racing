#pragma once
#include "entt/entity/registry.hpp"

namespace Apex
{
    class Scene
    {
    public:
        virtual ~Scene() = default;
        virtual void Init();
        virtual void Update(float) {}
        virtual void FixedUpdate(float) {}
        virtual void ShutDown() {}

        [[nodiscard]] entt::registry& GetRegistry() const { return *mRegistry; }

    protected:
        friend class ECS;
        friend class SceneManager;

    private:
        Ref<entt::registry> mRegistry;
        std::vector<entt::entity> mEntitiesToRemove;
    };
}  // namespace Apex