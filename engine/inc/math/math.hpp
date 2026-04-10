#pragma once
#include "core/components.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "core/ecs.hpp"

namespace Apex::Matrix
{
    inline glm::mat4 GetLocalMatrix(const Component::Transform& transform)
    {
        auto matrix = glm::mat4();
        ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform.m_position),
                                                glm::value_ptr(transform.m_rotation),
                                                glm::value_ptr(transform.m_scale),
                                                glm::value_ptr(matrix));
        return matrix;
    }

    inline glm::mat4 GetWorldMatrix(entt::entity entity)
    {
        const auto& transform = ECS::GetComponent<Component::Transform>(entity);
        const auto& hierarchy = ECS::TryGetComponent<Component::Hierarchy>(entity);

        glm::mat4 matrix;
        if (hierarchy && hierarchy->mParent != entt::null)
        {
            const auto parentMatrix = GetWorldMatrix(hierarchy->mParent);
            matrix = parentMatrix * GetLocalMatrix(transform);
        }
        else
            matrix = GetLocalMatrix(transform);

        return matrix;
    }

    inline glm::mat4 WorldToLocalMatrix(const glm::mat4& worldMatrix, entt::entity entity)
    {
        // Check if the entity has a parent
        Component::Hierarchy& hierarchy = ECS::GetComponent<Component::Hierarchy>(entity);
        if (hierarchy.mParent != entt::null)
        {
            glm::mat4 parentWorldMatrix = GetWorldMatrix(hierarchy.mParent);
            glm::mat4 inverseParentMatrix = inverse(parentWorldMatrix);
            return inverseParentMatrix * worldMatrix;
        }

        return worldMatrix;
    }
}  // namespace Apex::Matrix