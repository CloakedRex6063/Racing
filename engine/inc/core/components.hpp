#pragma once
#include "ecs.hpp"
#include "managers/resource_manager.hpp"

namespace Apex::Component
{
    struct Hierarchy
    {
        Entity mParent = entt::null;
        std::vector<Entity> mChildren;
    };

    struct Name
    {
        std::string mName;
        static void Inspector(Entity entity);
    };

    struct Transform
    {
        Transform() = default;
        Transform(Entity entity, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
        glm::vec3 m_position = glm::vec3();
        glm::vec3 m_rotation = glm::vec3();
        glm::vec3 m_scale = glm::vec3(1.f);

        glm::mat4 m_world_matrix = glm::mat4(1.f);
        Entity m_entity = entt::null;
        int m_transform_index = -1;

        static void Inspector(Entity entity);
        static void Update(Entity entity);

        glm::vec3 GetRight() { return glm::normalize(glm::vec3(m_world_matrix[0])); }
        glm::vec3 GetUp() { return glm::normalize(glm::vec3(m_world_matrix[1])); }
        glm::vec3 GetForward() { return glm::normalize(glm::vec3(m_world_matrix[2])); }

        glm::vec3 GetWorldPosition();
        glm::vec3 GetWorldRotation();
        glm::vec3 GetWorldScale();

        void SetLocalPosition(const glm::vec3& position);
        void SetLocalRotation(const glm::vec3& rotation);
        void SetLocalScale(const glm::vec3& scale);
        void SetLocalTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
    };

    struct AABB
    {
        glm::vec3 m_min = glm::vec3();
        glm::vec3 m_max = glm::vec3();
    };

    struct PointLight
    {
        PointLight();
        PointLight(glm::vec3 direction, float intensity, glm::vec3 color, float range);
        glm::vec3 position;
        float intensity = 1;
        glm::vec3 color = glm::vec3(1.0f);
        float range = 100.0f;
        int point_light_index;
    };

    struct DirectionalLight
    {
        DirectionalLight();
        DirectionalLight(glm::vec3 direction, float intensity, glm::vec3 color,
                         bool cast_shadows);
        glm::vec3 direction = glm::vec3(-0.3f, 1.0f, -0.2f);
        float intensity = 10.0f;
        glm::vec3 color = glm::vec3(1.0f);
        bool cast_shadows = false;
        int dir_light_index;

        void SetDirectionEuler(const glm::vec3& euler);
    };

    struct Renderable
    {
        Ref<Model> m_model;
        int m_mesh_index = 0;
    };

    struct Translucent
    {
        u32 dummy;
    };
} // namespace Apex::Component
