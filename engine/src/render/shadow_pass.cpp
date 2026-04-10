#include "render/shadow_pass.hpp"

#include "shader_data.hpp"
#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/renderer.hpp"
#include "managers/camera_manager.hpp"
#include "render/gen_draws.hpp"

Apex::ShadowPass::ShadowPass()
{
    const auto& renderer = g_engine.GetSystem<Renderer>();
    auto* context = renderer->GetContext();
    m_shadow_shader = Swift::GraphicsShaderBuilder(context)
                      .SetDSVFormat(Swift::Format::eD32F)
                      .SetMeshShader(shadows_mesh_main_code)
                      .SetPixelShader(shadows_pixel_main_code)
                      .SetDepthTestEnable(true)
                      .SetDepthWriteEnable(true)
                      .SetDepthTest(Swift::ComparisonFunc::eLess)
                      .SetPolygonMode(Swift::PolygonMode::eTriangle)
                      .SetName("PBR Shader")
                      .Build();
    m_shadow_texture = Swift::TextureBuilder(context, 4096, 4096).SetMipmapLevels(1).SetName("Shadow Texture").SetFlags(
                                                                      Swift::TextureFlags::eDepthStencil).
                                                                  SetFormat(Swift::Format::eD32F).Build();
    for (auto [index, dsv] : std::views::enumerate(m_shadow_texture_dsvs))
    {
        dsv = context->CreateTextureView(m_shadow_texture, {
                                             .type = Swift::TextureViewType::eDepthStencil,
                                             .base_mip_level = u32(index),
                                         });
    }
    for (auto [index, srv] : std::views::enumerate(m_shadow_texture_srvs))
    {
        srv = context->CreateTextureView(m_shadow_texture, {
                                             .type = Swift::TextureViewType::eShaderResource,
                                             .base_mip_level = u32(index),
                                         });
    }

    std::vector<u8> pass_constants(k_cbv_stride * k_shadow_cascades * 3, 0);
    for (u32 i = 0; i < k_shadow_cascades; ++i)
    {
        u32 cascade_index = i;
        for (u32 frame = 0; frame < 3; ++frame)
        {
            u32 offset = (i * 3 + frame) * k_cbv_stride;
            memcpy(pass_constants.data() + offset, &cascade_index, sizeof(u32));
        }
    }

    m_pass_constant_buffer = Swift::BufferBuilder(context, k_cbv_stride * k_shadow_cascades * 3).
                             SetData(pass_constants.data()).Build(); // 3 frames in flight, 4 cascades each
}

Apex::ShadowPass::~ShadowPass()
{
    const auto& renderer = g_engine.GetSystem<Renderer>();
    auto* context = renderer->GetContext();
    context->DestroyTexture(m_shadow_texture);
    for (const auto& dsv : m_shadow_texture_dsvs)
    {
        context->DestroyTextureView(dsv);
    }
    for (const auto& srv : m_shadow_texture_srvs)
    {
        context->DestroyTextureView(srv);
    }
}

void Apex::ShadowPass::Execute()
{
    // const auto& renderer = g_engine.GetSystem<Renderer>();
    // auto& render_graph = renderer->GetRenderGraph();
    //
    // GenerateCascades();
    // renderer->GetShadowBuffer()->Write(m_shadow_data.data(), 0, m_shadow_data.size() * sizeof(ShadowData));
    //
    // for (u32 i = 0; i < k_shadow_cascades; ++i)
    // {
    //     const glm::vec2 mip_dim = glm::max(glm::uvec2(1u), glm::uvec2(k_shadow_map_extent, k_shadow_map_extent) >> i);
    //     render_graph.AddRenderPass("Shadow Map Pass" + std::to_string(i), m_shadow_shader).SetDepthLoadOp(
    //                      Swift::LoadOp::eClear).WriteDepthStencil(m_shadow_texture_dsvs[i]).SetRenderExtents({
    //                      mip_dim.x, mip_dim.y
    //                  }).
    //                  SetExecute([this, renderer, i](Swift::ICommand* command)
    //                  {
    //                      const auto& gen_draw_pass = renderer->GetRenderPass<GenDrawsPass>();
    //                      auto* indirect_buffer = gen_draw_pass.GetIndirectCommandBuffer();
    //                      const u32 frame_index = renderer->GetContext()->GetFrameIndex();
    //
    //                      const u32 offset = (i * 3 + frame_index) * k_cbv_stride;
    //                      command->BindConstantBuffer(m_pass_constant_buffer, 2, offset);
    //
    //                      auto* count_buffer = gen_draw_pass.GetCountBuffer(frame_index);
    //                      auto* mesh_command_sig = gen_draw_pass.GetMeshCommandSignature();
    //                      command->TransitionBuffer(indirect_buffer, Swift::ResourceState::eIndirectArgument);
    //                      command->TransitionBuffer(count_buffer, Swift::ResourceState::eIndirectArgument);
    //                      command->DispatchMeshIndirect(mesh_command_sig, Renderer::k_max_draw_count,
    //                                                    indirect_buffer,
    //                                                    0, count_buffer, frame_index * sizeof(u32));
    //                  });
    // }
}


void Apex::ShadowPass::GenerateCascades()
{
    const auto camera_entity = g_engine.GetSystem<CameraManager>()->GetCurrentCamera();
    const auto camera = ECS::GetComponent<Component::Camera>(camera_entity);
    m_shadow_cascade_levels = {
        camera.mFar / 50.0f, //camera.mFar / 25.0f, camera.mFar / 10.0f, camera.mFar / 2.0f
    };
    const auto shadowMatrices = GetLightSpaceMatrices();
    for (auto [i, shadowData] : std::views::enumerate(m_shadow_data))
    {
        shadowData.shadow_light_matrix = shadowMatrices[i];
        shadowData.cascade_plane_distance = m_shadow_cascade_levels[i];
        shadowData.shadow_map_index = m_shadow_texture_srvs[i]->GetDescriptorIndex();
    }
}

// https://learnopengl.com/code_viewer_gh.php?code=src/8.guest/2021/2.csm/shadow_mapping.cpp
std::vector<glm::vec4> Apex::ShadowPass::GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
    const auto inv = glm::inverse(proj * view);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    return frustumCorners;
}

glm::mat4 Apex::ShadowPass::GetLightSpaceMatrix(const float near_plane, const float far_plane)
{
    const auto camera_entity = g_engine.GetSystem<CameraManager>()->GetCurrentCamera();
    const auto camera = ECS::GetComponent<Component::Camera>(camera_entity);

    const auto viewportSize = g_engine.GetSystem<Device>()->GetWindowSize();
    auto proj = glm::perspectiveZO(
        glm::radians(camera.mFov), (float)viewportSize.x / (float)viewportSize.y, near_plane,
        far_plane);
    proj[1][1] *= -1;
    const auto view = glm::inverse(ECS::GetComponent<Component::Transform>(camera_entity).m_world_matrix);
    std::vector<glm::vec4> frustum_corners = GetFrustumCornersWorldSpace(proj, view);

    auto center = glm::vec3(0, 0, 0);
    for (const auto& v : frustum_corners)
    {
        center += glm::vec3(v);
    }
    center /= frustum_corners.size();

    const auto dir_light = *ECS::GetRegistry().view<Component::DirectionalLight>().begin();
    const auto lightDir = ECS::GetComponent<Component::Transform>(dir_light).GetForward();
    const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : frustum_corners)
    {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    // Tune this parameter according to the scene
    constexpr float zMult = 10000.0f;
    if (minZ < 0)
    {
        minZ *= zMult;
    }
    else
    {
        minZ /= zMult;
    }
    if (maxZ < 0)
    {
        maxZ /= zMult;
    }
    else
    {
        maxZ *= zMult;
    }

    glm::mat4 lightProjection = glm::orthoZO(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

std::vector<glm::mat4> Apex::ShadowPass::GetLightSpaceMatrices() const
{
    const auto camera_entity = g_engine.GetSystem<CameraManager>()->GetCurrentCamera();
    const auto camera = ECS::GetComponent<Component::Camera>(camera_entity);
    std::vector<glm::mat4> ret;
    for (size_t i = 0; i < k_shadow_cascades + 1; ++i)
    {
        if (i == 0)
        {
            ret.push_back(GetLightSpaceMatrix(camera.mNear, m_shadow_cascade_levels[i]));
        }
        else if (i < m_shadow_cascade_levels.size())
        {
            ret.push_back(GetLightSpaceMatrix(m_shadow_cascade_levels[i - 1], m_shadow_cascade_levels[i]));
        }
        else
        {
            ret.push_back(GetLightSpaceMatrix(m_shadow_cascade_levels[i - 1], camera.mFar));
        }
    }
    return ret;
}
