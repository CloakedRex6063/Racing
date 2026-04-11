#include "render/depth_prepass.hpp"
#include "shader_data.hpp"
#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/renderer.hpp"
#include "render/gen_draws.hpp"

Apex::DepthPrepass::DepthPrepass()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    m_depth_shader = Swift::GraphicsShaderBuilder(context)
                     .SetDSVFormat(Swift::Format::eD32F)
                     .SetAmplificationShader(depth_prepass_ampl_main_code)
                     .SetMeshShader(depth_prepass_mesh_main_code)
                     .SetPixelShader(depth_prepass_pixel_main_code)
                     .SetDepthTestEnable(true)
                     .SetDepthWriteEnable(true)
                     .SetDepthTest(Swift::ComparisonFunc::eLess)
                     .SetPolygonMode(Swift::PolygonMode::eTriangle)
                     .SetName("Depth Prepass Shader")
                     .Build();
}

Apex::DepthPrepass::~DepthPrepass()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    context->DestroyShader(m_depth_shader);
}

void Apex::DepthPrepass::Execute()
{
    const auto renderer = g_engine.GetSystem<Renderer>();
    auto& render_graph = renderer->GetRenderGraph();
    const auto* context = renderer->GetContext();
    const auto depth_target_view = renderer->GetDepthTargetView();
    const auto window_size = g_engine.GetSystem<Device>()->GetWindowSize();

    const u32 frame_index = renderer->GetContext()->GetFrameIndex();
    const u32 count_offset = frame_index * sizeof(u32);
    u32 opaque_count = renderer->GetOpaqueDraws().size();
    render_graph.AddRenderPass("Depth Prepass", m_depth_shader)
                .WriteDepthStencil(depth_target_view).SetDepthLoadOp(Swift::LoadOp::eClear)
                .SetRenderExtents(Swift::Float2(window_size.x, window_size.y))
                .SetExecute(
                    [this, renderer, opaque_count, count_offset](
                    Swift::ICommand* command)
                    {
                        const auto& gen_draw_pass = renderer->GetRenderPass<GenDrawsPass>();
                        const auto indirect_buffer = gen_draw_pass.GetIndirectCommandBuffer();
                        const auto mesh_command_sig = gen_draw_pass.GetMeshCommandSignature();
                        command->TransitionBuffer(indirect_buffer,
                                                  Swift::ResourceState::eIndirectArgument);
                        command->TransitionBuffer(gen_draw_pass.GetCountBuffer(),
                                                  Swift::ResourceState::eIndirectArgument);
                        command->ExecuteIndirect(mesh_command_sig, opaque_count,
                                                 indirect_buffer, 0,
                                                 gen_draw_pass.GetCountBuffer(), count_offset);
                    });
}
