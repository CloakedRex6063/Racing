#include "core/components.hpp"

#include "core/engine.hpp"
#include "core/renderer.hpp"
#include "math/math.hpp"

Apex::Component::Transform::Transform(const Entity entity, const glm::vec3 position, const glm::vec3 rotation,
                                      const glm::vec3 scale) : m_position(position), m_rotation(rotation),
                                                               m_scale(scale), m_entity(entity)
{
}

void Apex::Component::Transform::Inspector(Entity entity)
{
    auto& transform = ECS::GetComponent<Transform>(entity);
    ImGui::DragFloat3("Position", glm::value_ptr(transform.m_position));
    ImGui::DragFloat3("Rotation", glm::value_ptr(transform.m_rotation));
    ImGui::DragFloat3("Scale", glm::value_ptr(transform.m_scale));
    ECS::GetRegistry().patch<Transform>(entity);
}

void Apex::Component::Transform::Update(Entity entity)
{
    auto& transform = ECS::GetComponent<Transform>(entity);
    transform.m_world_matrix = Matrix::GetWorldMatrix(entity);
    if (ECS::HasComponent<Renderable>(entity))
    {
        const auto& renderer = g_engine.GetSystem<Renderer>();
        renderer->UpdateTransform(transform.m_transform_index, transform.m_world_matrix);
    }

    const auto& hierarchy = ECS::GetComponent<Hierarchy>(entity);
    for (const auto& child : hierarchy.mChildren)
    {
        Update(child);
    }
}

glm::vec3 Apex::Component::Transform::GetWorldPosition() { return m_world_matrix[3]; }

glm::vec3 Apex::Component::Transform::GetWorldRotation()
{
    glm::vec3 world_rotation{};
    world_rotation[0] = glm::degrees(atan2f(m_world_matrix[1][2], m_world_matrix[2][2]));
    world_rotation[1] =
        glm::degrees(atan2f(-m_world_matrix[0][2],
                            sqrtf(m_world_matrix[1][2] * m_world_matrix[1][2] + m_world_matrix[2][2] * m_world_matrix[2]
                                [2])));
    world_rotation[2] = glm::degrees(atan2f(m_world_matrix[0][1], m_world_matrix[0][0]));
    return world_rotation;
}

glm::vec3 Apex::Component::Transform::GetWorldScale()
{
    glm::vec3 world_scale{};
    world_scale.x = length(glm::vec3(m_world_matrix[0]));
    world_scale.y = length(glm::vec3(m_world_matrix[1]));
    world_scale.z = length(glm::vec3(m_world_matrix[2]));
    return world_scale;
}

void Apex::Component::Transform::SetLocalPosition(const glm::vec3& position)
{
    m_position = position;
    Update(m_entity); // Temporary until i can add reflection in
    ECS::GetRegistry().patch<Transform>(m_entity);
}

void Apex::Component::Transform::SetLocalRotation(const glm::vec3& rotation)
{
    m_rotation = rotation;
    Update(m_entity); // Temporary until i can add reflection in
    ECS::GetRegistry().patch<Transform>(m_entity);
}

void Apex::Component::Transform::SetLocalScale(const glm::vec3& scale)
{
    m_scale = scale;
    Update(m_entity); // Temporary until i can add reflection in
    ECS::GetRegistry().patch<Transform>(m_entity);
}

void Apex::Component::Transform::SetLocalTransform(const glm::vec3& position, const glm::vec3& rotation,
                                                   const glm::vec3& scale)
{
    m_position = position;
    m_rotation = rotation;
    m_scale = scale;
    Update(m_entity); // Temporary until i can add reflection in
    ECS::GetRegistry().patch<Transform>(m_entity);
}

Apex::Component::PointLight::PointLight()
{
    const auto& renderer = g_engine.GetSystem<Renderer>();
    point_light_index = renderer->AddPointLight({
        .position = position, .intensity = intensity, .color = color, .range = range,
    });
}

Apex::Component::PointLight::PointLight(const glm::vec3 direction, const float intensity, const glm::vec3 color,
                                        const float range)
{
    const auto& renderer = g_engine.GetSystem<Renderer>();
    point_light_index = renderer->AddPointLight({
        .position = position, .intensity = intensity, .color = color, .range = range
    });
}

Apex::Component::DirectionalLight::DirectionalLight()
{
    const auto& renderer = g_engine.GetSystem<Renderer>();
    dir_light_index = renderer->AddDirLight({
        .direction = direction, .intensity = intensity, .color = color, .cast_shadows = cast_shadows,
    });
}

Apex::Component::DirectionalLight::DirectionalLight(const glm::vec3 direction, const float intensity,
                                                    const glm::vec3 color,
                                                    const bool cast_shadows) : direction(direction),
                                                                               intensity(intensity), color(color),
                                                                               cast_shadows(cast_shadows)
{
    const auto& renderer = g_engine.GetSystem<Renderer>();
    dir_light_index = renderer->AddDirLight({
        .direction = direction, .intensity = intensity, .color = color, .cast_shadows = cast_shadows,
    });
}

void Apex::Component::DirectionalLight::SetDirectionEuler(const glm::vec3& euler)
{
    const glm::mat4 rot =
        glm::yawPitchRoll(glm::radians(euler.y), glm::radians(euler.x), glm::radians(euler.z));
    constexpr auto forward = glm::vec3(0.0f, 0.0f, -1.0f);
    direction = glm::normalize(glm::vec3(rot * glm::vec4(forward, 0.0f)));
}
