#include "render/geom_pass.hpp"

#include "shader_data.hpp"
#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/renderer.hpp"
#include "render/gen_draws.hpp"

Apex::GeomPass::GeomPass()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    m_opaque_shader = Swift::GraphicsShaderBuilder(context)
                      .AddRTVFormat(Swift::Format::eRGBA8_UNORM)
                      .SetDSVFormat(Swift::Format::eD32F)
                      .SetMeshShader(geom_mesh_main_code)
                      .SetPixelShader(geom_pixel_main_code)
                      .SetDepthTestEnable(true)
                      .SetDepthWriteEnable(true)
                      .SetDepthTest(Swift::ComparisonFunc::eLess)
                      .SetPolygonMode(Swift::PolygonMode::eTriangle)
                      .SetName("PBR Shader")
                      .Build();
    m_translucent_shader = Swift::GraphicsShaderBuilder(context)
                           .AddRTVFormat(Swift::Format::eRGBA8_UNORM)
                           .AddBlendInfo(Swift::BlendInfo{
                               .blend_enable = true,
                               .logic_op_enable = false,
                               .src_color_factor = Swift::BlendFactor::eSrcAlpha,
                               .dst_color_factor = Swift::BlendFactor::eInvSrcAlpha,
                               .src_alpha_factor = Swift::BlendFactor::eOne,
                               .dst_alpha_factor = Swift::BlendFactor::eInvSrcAlpha,
                               .color_blend_op = Swift::BlendOp::eAdd,
                               .alpha_blend_op = Swift::BlendOp::eAdd,
                               .logic_op = Swift::LogicOp::eNoOp
                           })
                           .SetDSVFormat(Swift::Format::eD32F)
                           .SetMeshShader(geom_mesh_main_code)
                           .SetPixelShader(geom_pixel_main_code)
                           .SetDepthTestEnable(true)
                           .SetDepthWriteEnable(false)
                           .SetDepthTest(Swift::ComparisonFunc::eLess)
                           .SetPolygonMode(Swift::PolygonMode::eTriangle)
                           .SetName("Transparent PBR Shader")
                           .Build();
}

Apex::GeomPass::~GeomPass()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    context->DestroyShader(m_opaque_shader);
}

void Apex::GeomPass::Execute()
{
    const auto renderer = g_engine.GetSystem<Renderer>();
    auto& render_graph = renderer->GetRenderGraph();
    const auto* context = renderer->GetContext();
    const auto depth_target_view = renderer->GetDepthTargetView();
    const auto window_size = g_engine.GetSystem<Device>()->GetWindowSize();

    auto AddGeometryPass = [&](const std::string& name, Swift::IShader* shader, u32 indirect_offset, u32 count_offset,
                               u32 max_draws,
                               bool write_depth)
    {
        auto& pass = render_graph.AddRenderPass(name, shader)
                                 .SetRenderExtents(Swift::Float2(window_size.x, window_size.y))
                                 .WriteRenderTarget(context->GetCurrentRenderTarget());

        if (write_depth)
            pass.WriteDepthStencil(depth_target_view).SetDepthLoadOp(Swift::LoadOp::eClear);
        else
            pass.WriteDepthStencil(depth_target_view);

        pass.SetExecute([this, renderer, indirect_offset, count_offset, max_draws](Swift::ICommand* command)
        {
            const auto& gen_draw_pass = renderer->GetRenderPass<GenDrawsPass>();
            auto indirect_buffer = gen_draw_pass.GetIndirectCommandBuffer();
            auto mesh_command_sig = gen_draw_pass.GetMeshCommandSignature();
            command->TransitionBuffer(indirect_buffer, Swift::ResourceState::eIndirectArgument);
            command->TransitionBuffer(gen_draw_pass.GetCountBuffer(), Swift::ResourceState::eIndirectArgument);
            command->DispatchMeshIndirect(mesh_command_sig, max_draws,
                                          indirect_buffer, indirect_offset,
                                          gen_draw_pass.GetCountBuffer(), count_offset);
        });
    };

    const u32 frame_index = renderer->GetContext()->GetFrameIndex();
    u32 opaque_count = renderer->GetOpaqueDraws().size();
    u32 translucent_count = renderer->GetTranslucentDraws().size();
    AddGeometryPass("Geometry Pass", m_opaque_shader, 0, frame_index * sizeof(u32), opaque_count, true);
    AddGeometryPass("Translucent Pass", m_translucent_shader,
                    opaque_count * sizeof(IndirectCommand), 3 * sizeof(u32) + frame_index * sizeof(u32),
                    translucent_count, false);
}
