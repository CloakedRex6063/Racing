#include "core/renderer.hpp"
#include "core/device.hpp"
#include "core/engine.hpp"
#include "managers/camera_manager.hpp"
#include "render/depth_prepass.hpp"
#include "render/gen_draws.hpp"
#include "render/geom_pass.hpp"
#include "render/shadow_pass.hpp"
#include "render/skybox_pass.hpp"


void Apex::Renderer::Init()
{
    const auto& device = g_engine.GetSystem<Device>();
    const glm::uvec2 window_size = device->GetWindowSize();
    m_context = Swift::CreateContext({
        .width = window_size.x, .height = window_size.y, .native_window_handle = device->GetNativeWindow(),
    });
    m_depth_target = Swift::TextureBuilder(m_context, window_size.x, window_size.y)
                     .SetFormat(Swift::Format::eD32F)
                     .SetFlags(Swift::TextureFlags::eDepthStencil)
                     .Build();
    m_depth_target_view = m_context->CreateTextureView(m_depth_target,
                                                       {
                                                           .type = Swift::TextureViewType::eDepthStencil,
                                                       });
    BuildSamplers();
    BuildBuffers();
    AddRenderPass<GenDrawsPass>();
    AddRenderPass<DepthPrepass>();
    AddRenderPass<ShadowPass>();
    AddRenderPass<GeomPass>();
    AddRenderPass<SkyboxPass>();

    device->AddResizeCallback([&](const glm::uvec2& size)
    {
        m_context->GetGraphicsQueue()->WaitIdle();
        m_context->ResizeBuffers(size.x, size.y);

        m_context->DestroyTexture(m_depth_target);
        m_context->DestroyTextureView(m_depth_target_view);

        m_depth_target = Swift::TextureBuilder(m_context, size.x, size.y)
                         .SetFormat(Swift::Format::eD32F)
                         .SetFlags(Swift::TextureFlags::eDepthStencil)
                         .Build();
        m_depth_target_view = m_context->CreateTextureView(m_depth_target,
                                                           {
                                                               .type = Swift::TextureViewType::eDepthStencil,
                                                           });
    });
}

void Apex::Renderer::Update(float)
{
    m_context->NewFrame();
    auto* command = m_context->GetCurrentCommand();
    m_render_graph.NewFrame(command);

    UpdateGlobalConstantBuffer();
    UpdateTransformBuffer();

    auto MakeDrawData = [](std::vector<DrawData>& draw_data, const Component::Renderable& renderable,
                           const Component::Transform& transform)
    {
        const auto& mesh = renderable.m_model->m_meshes[renderable.m_mesh_index];
        draw_data.emplace_back(DrawData{
            .meshlet_count = mesh.meshlet_count,
            .meshlet_offset = mesh.meshlet_offset,
            .transform_index = static_cast<u32>(transform.m_transform_index),
            .material_index = mesh.material_index,
        });
    };

    if (m_draw_buffer_dirty)
    {
        m_opaque_draws.clear();
        m_translucent_draws.clear();
        const auto opaque_view = ECS::GetRegistry().view<
            Component::Renderable, Component::Transform>(entt::exclude<Component::Translucent>);
        const auto translucent_view = ECS::GetRegistry().view<
            Component::Renderable, Component::Transform, Component::Translucent>();


        for (auto [entity, renderable, transform] : opaque_view.each())
        {
            MakeDrawData(m_opaque_draws, renderable, transform);
        }

        for (auto [entity, renderable, transform, translucent] : translucent_view.each())
        {
            MakeDrawData(m_translucent_draws, renderable, transform);
        }

        m_draw_buffer->Write(m_opaque_draws.data(), 0, sizeof(DrawData) * m_opaque_draws.size());
        m_draw_buffer->Write(m_translucent_draws.data(), m_opaque_draws.size() * sizeof(DrawData),
                             sizeof(DrawData) * m_translucent_draws.size());

        m_draw_buffer_dirty = false;
    }

    const u32 frame_index = m_context->GetFrameIndex();
    auto* swapchain_texture = m_context->GetCurrentSwapchainTexture();
    auto* swapchain_target = m_context->GetCurrentRenderTarget();
    command->Begin();
    command->TransitionImage(swapchain_texture, Swift::ResourceState::eRenderTarget);
    command->ClearRenderTarget(swapchain_target, {1.0f, 0.0f, 0.0f, 1.0f});
    command->BindConstantBuffer(m_global_constant_buffer, 1, k_constant_buffer_aligned_size * frame_index);

    for (const auto& pass : m_render_passes)
    {
        pass->Execute();
    }

    m_render_graph.Execute();

    command->TransitionImage(swapchain_texture, Swift::ResourceState::eCommon);
    command->End();
    m_context->Present(false);
}

void Apex::Renderer::Shutdown()
{
    GetContext()->GetGraphicsQueue()->WaitIdle();

    m_render_passes.clear();

    m_context->DestroyBuffer(m_vertex_buffer);
    m_context->DestroyBuffer(m_triangle_buffer);
    m_context->DestroyBuffer(m_meshlet_buffer);
    m_context->DestroyBuffer(m_transform_buffer);
    m_context->DestroyBuffer(m_position_buffer);
    m_context->DestroyBuffer(m_material_buffer);
    m_context->DestroyBuffer(m_global_constant_buffer);
    m_context->DestroyBuffer(m_point_light_buffer);
    m_context->DestroyBuffer(m_dir_light_buffer);

    m_context->DestroyBufferView(m_vertex_buffer_view);
    m_context->DestroyBufferView(m_triangle_buffer_view);
    m_context->DestroyBufferView(m_meshlet_buffer_view);
    m_context->DestroyBufferView(m_transform_buffer_view);
    m_context->DestroyBufferView(m_position_buffer_view);
    m_context->DestroyBufferView(m_material_buffer_view);
    m_context->DestroyBufferView(m_point_light_buffer_view);
    m_context->DestroyBufferView(m_dir_light_buffer_view);

    m_context->DestroySampler(m_bilinear_sampler);
    m_context->DestroySampler(m_nearest_sampler);
}

u32 Apex::Renderer::AddTransform(const glm::mat4& transform)
{
    if (!m_free_transforms.empty())
    {
        const u32 index = m_free_transforms.back();
        m_free_transforms.pop_back();

        GPUTransform new_transform
        {
            .transform = transform,
            .inv_transform = glm::transpose(glm::inverse(transform)),
        };

        m_pending_transforms[index] = new_transform;
        return index;
    }

    GPUTransform new_transform
    {
        .transform = transform,
        .inv_transform = glm::transpose(glm::inverse(transform)),
    };

    const u32 index = m_transform_count++;
    m_pending_transforms[index] = new_transform;
    return index;
}

void Apex::Renderer::UpdateTransform(const u32 transform_index, const glm::mat4& transform)
{
    GPUTransform new_transform
    {
        .transform = transform,
        .inv_transform = glm::transpose(glm::inverse(transform)),
    };
    m_pending_transforms[transform_index] = new_transform;
}

u32 Apex::Renderer::AddDirLight(const GPUDirectionalLight& dir_light)
{
    if (!m_free_dir_lights.empty())
    {
        const u32 index = m_free_dir_lights.back();
        m_free_dir_lights.pop_back();
        GetDirLightBuffer()->Write(&dir_light, index, sizeof(GPUDirectionalLight));
        return index;
    }

    const u32 index = m_dir_light_count++;
    GetDirLightBuffer()->Write(&dir_light, index, sizeof(GPUDirectionalLight));
    return index;
}

u32 Apex::Renderer::AddPointLight(const GPUPointLight& point_light)
{
    if (!m_free_point_lights.empty())
    {
        const u32 index = m_free_point_lights.back();
        m_free_point_lights.pop_back();
        GetPointLightBuffer()->Write(&point_light, 0, sizeof(GPUPointLight));
        return index;
    }

    const u32 index = m_point_light_count++;
    GetPointLightBuffer()->Write(&point_light, 0, sizeof(GPUPointLight));
    return index;
}

std::expected<void, Apex::Renderer::Error> Apex::Renderer::SetSkybox(const fs::path& path)
{
    m_context->GetGraphicsQueue()->WaitIdle();

    const auto& resource_manager = g_engine.GetSystem<ResourceManager>();

    auto exp_skybox_texture = resource_manager->LoadTexture(path.string());
    if (!exp_skybox_texture)
    {
        return std::unexpected(Error::eFailedToLoadSkybox);
    }

    fs::path diff_path = path;
    diff_path.replace_filename(path.stem().string() + "_diff" + path.extension().string());

    fs::path spec_path = path;
    spec_path.replace_filename(path.stem().string() + "_spec" + path.extension().string());

    fs::path lut_path = path;
    lut_path.replace_filename(path.stem().string() + "_lut" + path.extension().string());

    auto exp_diff_texture = resource_manager->LoadTexture(diff_path.string());
    if (!exp_diff_texture)
    {
        return std::unexpected(Error::eFailedToLoadDiffuseIBL);
    }

    auto exp_ibl_texture = resource_manager->LoadTexture(spec_path.string());
    if (!exp_ibl_texture)
    {
        return std::unexpected(Error::eFailedToLoadSpecularIBL);
    }

    auto exp_lut_texture = resource_manager->LoadTexture(lut_path.string());
    if (!exp_lut_texture)
    {
        return std::unexpected(Error::eFailedToLoadLut);
    }

    GetRenderPass<SkyboxPass>().SetSkybox(std::move(exp_skybox_texture.value()));

    if (m_diffuse_ibl_texture)
    {
        m_context->DestroyTexture(m_diffuse_ibl_texture);
        m_context->DestroyTextureView(m_diffuse_ibl_texture_view);
    }
    m_diffuse_ibl_texture = exp_diff_texture->m_texture;
    m_diffuse_ibl_texture_view = exp_diff_texture->m_texture_view;

    if (m_specular_ibl_texture)
    {
        m_context->DestroyTexture(m_specular_ibl_texture);
        m_context->DestroyTextureView(m_specular_ibl_texture_view);
    }
    m_specular_ibl_texture = exp_ibl_texture->m_texture;
    m_specular_ibl_texture_view = exp_ibl_texture->m_texture_view;

    if (m_lut_texture)
    {
        m_context->DestroyTexture(m_lut_texture);
        m_context->DestroyTextureView(m_lut_texture_view);
    }
    m_lut_texture = exp_lut_texture->m_texture;
    m_lut_texture_view = exp_lut_texture->m_texture_view;

    return {};
}

void Apex::Renderer::BuildBuffers()
{
    m_position_buffer = Swift::BufferBuilder(m_context, k_max_vertex_count * sizeof(glm::uvec2)).SetName(
            "Position Buffer").
        Build();
    m_position_buffer_view = m_context->CreateBufferView(m_position_buffer, {
                                                             .num_elements = k_max_vertex_count,
                                                             .element_size = sizeof(glm::uvec2),
                                                         });
    m_vertex_buffer = Swift::BufferBuilder(m_context, k_max_vertex_count * sizeof(Vertex)).SetName("Vertex Buffer").
        Build();
    m_vertex_buffer_view = m_context->CreateBufferView(m_vertex_buffer, {
                                                           .num_elements = k_max_vertex_count,
                                                           .element_size = sizeof(Vertex),
                                                       });
    m_triangle_buffer = Swift::BufferBuilder(m_context, k_max_triangle_count * sizeof(uint32_t)).SetName(
            "Triangle Buffer").
        Build();
    m_triangle_buffer_view = m_context->CreateBufferView(m_triangle_buffer, {
                                                             .num_elements = k_max_triangle_count,
                                                             .element_size = sizeof(uint32_t),
                                                         });
    m_meshlet_buffer = Swift::BufferBuilder(m_context, k_max_meshlet_count * sizeof(Meshlet)).SetName("Meshlet Buffer").
        Build();
    m_meshlet_buffer_view = m_context->CreateBufferView(m_meshlet_buffer, {
                                                            .num_elements = k_max_meshlet_count,
                                                            .element_size = sizeof(Meshlet),
                                                        });
    m_cull_data_buffer = Swift::BufferBuilder(m_context, k_max_meshlet_count * sizeof(CullData)).SetName(
            "Cull Data Buffer").
        Build();
    m_cull_data_buffer_view = m_context->CreateBufferView(m_cull_data_buffer, {
                                                              .num_elements = k_max_meshlet_count,
                                                              .element_size = sizeof(CullData),
                                                          });
    m_transform_buffer = Swift::BufferBuilder(m_context, k_max_draw_count * sizeof(GPUTransform)).SetName(
            "Transform Buffer").
        Build();
    m_transform_buffer_view = m_context->CreateBufferView(m_transform_buffer, {
                                                              .num_elements = k_max_draw_count,
                                                              .element_size = sizeof(GPUTransform),
                                                          });
    m_draw_buffer = Swift::BufferBuilder(m_context, k_max_draw_count * sizeof(DrawData)).SetName(
            "Draw Data Buffer").
        Build();
    m_draw_buffer_view = m_context->CreateBufferView(m_draw_buffer, {
                                                         .num_elements = k_max_draw_count,
                                                         .element_size = sizeof(DrawData),
                                                     });
    m_material_buffer = Swift::BufferBuilder(m_context, k_max_material_count * sizeof(Material)).
                        SetName("Material Buffer").Build();
    m_material_buffer_view = m_context->CreateBufferView(m_material_buffer, {
                                                             .num_elements = k_max_material_count,
                                                             .element_size = sizeof(Material),
                                                         });
    m_point_light_buffer = Swift::BufferBuilder(m_context, k_max_point_light_count * sizeof(GPUPointLight)).SetName(
        "Point Light Buffer").Build();
    m_point_light_buffer_view = m_context->CreateBufferView(m_point_light_buffer, {
                                                                .num_elements = k_max_point_light_count,
                                                                .element_size = sizeof(GPUPointLight),
                                                            });
    m_dir_light_buffer = Swift::BufferBuilder(m_context, k_max_dir_light_count * sizeof(GPUDirectionalLight)).SetName(
        "Directional Light Buffer").Build();
    m_dir_light_buffer_view = m_context->CreateBufferView(m_dir_light_buffer, {
                                                              .num_elements = k_max_dir_light_count,
                                                              .element_size = sizeof(GPUDirectionalLight),
                                                          });

    m_shadow_buffer = Swift::BufferBuilder(m_context, 4 * sizeof(ShadowData)).SetName(
        "Shadow Buffer").Build();
    m_shadow_buffer_view = m_context->CreateBufferView(m_shadow_buffer, {
                                                           .num_elements = 4,
                                                           .element_size = sizeof(ShadowData),
                                                       });

    m_global_constant_buffer = Swift::BufferBuilder(m_context, k_constant_buffer_aligned_size * 3).SetName(
        "Global Constant Buffer").Build();
}

void Apex::Renderer::BuildSamplers()
{
    m_bilinear_sampler = Swift::SamplerBuilder(m_context).Build();
    m_nearest_sampler =
        Swift::SamplerBuilder(m_context).SetMinFilter(Swift::Filter::eNearest).SetMagFilter(Swift::Filter::eNearest).
                                         Build();
}

void Apex::Renderer::UpdateGlobalConstantBuffer() const
{
    const auto camera_manager = g_engine.GetSystem<CameraManager>();
    const auto view = camera_manager->GetViewMatrix();
    const auto proj = camera_manager->GetProjectionMatrix();

    const auto view_proj = proj * view;
    glm::mat4 vp_t = glm::transpose(view_proj);

    // Gribb-Hartmann plane extraction
    auto normalize_plane = [](glm::vec4 p)
    {
        return p / glm::length(glm::vec3(p));
    };

    glm::vec4 left   = normalize_plane(vp_t[3] + vp_t[0]);
    glm::vec4 right  = normalize_plane(vp_t[3] - vp_t[0]);
    glm::vec4 bottom = normalize_plane(vp_t[3] + vp_t[1]);
    glm::vec4 top    = normalize_plane(vp_t[3] - vp_t[1]);
    glm::vec4 near_p = normalize_plane(vp_t[2]);
    glm::vec4 far_p  = normalize_plane(vp_t[3] - vp_t[2]);


    auto current_cam = camera_manager->GetCurrentCamera();
    const auto position = ECS::GetComponent<Component::Transform>(current_cam).m_position;
    const auto camera = ECS::GetComponent<Component::Camera>(current_cam);
    const GlobalConstants constants{
        .view = view,
        .proj = proj,
        .view_proj = view_proj,
        .frustum = {left, right, bottom, top, near_p, far_p},
        .cam_pos = position,
        .cam_near = camera.mNear,
        .cam_far = camera.mFar,
        .position_buffer_index = m_position_buffer_view->GetDescriptorIndex(),
        .vertex_buffer_index = m_vertex_buffer_view->GetDescriptorIndex(),
        .triangle_buffer_index = m_triangle_buffer_view->GetDescriptorIndex(),
        .meshlet_buffer_index = m_meshlet_buffer_view->GetDescriptorIndex(),
        .transform_buffer_index = m_transform_buffer_view->GetDescriptorIndex(),
        .material_buffer_index = m_material_buffer_view->GetDescriptorIndex(),
        .cull_buffer_index = m_cull_data_buffer_view->GetDescriptorIndex(),
        .shadow_buffer_index = m_shadow_buffer_view->GetDescriptorIndex(),
        .point_buffer_index = m_point_light_buffer_view->GetDescriptorIndex(),
        .point_light_count = m_point_light_count,
        .dir_buffer_index = m_dir_light_buffer_view->GetDescriptorIndex(),
        .dir_light_count = m_dir_light_count,
        .diffuse_ibl_index = m_diffuse_ibl_texture_view->GetDescriptorIndex(),
        .specular_ibl_index = m_specular_ibl_texture_view->GetDescriptorIndex(),
        .lut_ibl_index = m_lut_texture_view->GetDescriptorIndex(),
        .bilinear_sampler_index = m_bilinear_sampler->GetDescriptorIndex(),
        .nearest_sampler_index = m_nearest_sampler->GetDescriptorIndex(),
    };
    const u32 frame_index = m_context->GetFrameIndex();
    m_global_constant_buffer->Write(&constants, k_constant_buffer_aligned_size * frame_index, sizeof(GlobalConstants));
}

void Apex::Renderer::UpdateTransformBuffer()
{
    for (const auto& [index, matrix] : m_pending_transforms)
    {
        m_transform_buffer->Write(&matrix, index * sizeof(GPUTransform), sizeof(GPUTransform));
    }
    m_pending_transforms.clear();
}
