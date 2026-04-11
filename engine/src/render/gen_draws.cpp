#include "render/gen_draws.hpp"
#include "shader_data.hpp"
#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/renderer.hpp"

Apex::GenDrawsPass::GenDrawsPass()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();

    m_fill_shader = Swift::ComputeShaderBuilder(context, fill_draws_main_code).SetName("Compute Shader").Build();

    std::vector<Swift::IndirectArgument> indirect_arguments(2);
    indirect_arguments[0].type = Swift::IndirectArgumentType::ePushConstant;
    indirect_arguments[0].size = sizeof(u32) * 4;

    indirect_arguments[1].type = Swift::IndirectArgumentType::eMeshDispatch;

    m_mesh_command_signature = context->CreateCommandSignature(indirect_arguments);

    m_indirect_buffer = Swift::BufferBuilder(context, sizeof(IndirectMeshCommand) * Renderer::k_max_draw_count).
                        SetBufferFlags(Swift::BufferFlags::eUnorderedAccess).
                        SetName(
                            "Indirect Command Buffer").Build();
    m_indirect_buffer_view = context->CreateBufferView(m_indirect_buffer, {
                                                           .num_elements = Renderer::k_max_draw_count,
                                                           .element_size = sizeof(DrawData),
                                                       });
    m_count_buffer = Swift::BufferBuilder(context, sizeof(u32) * 3 * 2). //3 frames in flight, opaque + transparent
                     SetBufferFlags(Swift::BufferFlags::eUnorderedAccess).SetName("Draw Count Buffer").
                     Build();
    for (auto [index, count_buffer] : std::views::enumerate(m_opaque_count_buffer_views))
    {
        count_buffer = context->CreateBufferView(m_count_buffer, {
                                                     .first_element = static_cast<u32>(index),
                                                     .num_elements = 1,
                                                     .element_size = sizeof(u32),
                                                 });
    }
    for (auto [index, count_buffer] : std::views::enumerate(m_translucent_count_buffer_views))
    {
        count_buffer = context->CreateBufferView(m_count_buffer, {
                                                     .first_element = static_cast<u32>(index) + 3,
                                                     .num_elements = 1,
                                                     .element_size = sizeof(u32),
                                                 });
    }
}

Apex::GenDrawsPass::~GenDrawsPass()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    context->DestroyShader(m_fill_shader);
    context->DestroyBuffer(m_indirect_buffer);
    context->DestroyBuffer(m_count_buffer);
    context->DestroyCommandSignature(m_mesh_command_signature);
}

void Apex::GenDrawsPass::Execute()
{
    const auto renderer = g_engine.GetSystem<Renderer>();
    auto& render_graph = renderer->GetRenderGraph();
    const auto* context = renderer->GetContext();
    u32 frame_index = context->GetFrameIndex();

    u32 count = 0;
    m_count_buffer->Write(&count, frame_index * sizeof(u32), sizeof(u32));

    auto AddFillPass = [&](const std::string& name, u32 mesh_count, u32 base_offset,
                           Swift::IBufferView* indirect_cmd_view, Swift::IBufferView* count_view)
    {
        render_graph.AddComputePass(name, m_fill_shader)
                    .Write(indirect_cmd_view)
                    .Write(count_view)
                    .Read(renderer->GetDrawDataBufferView())
                    .SetExecute([this, mesh_count, base_offset, indirect_cmd_view, count_view](Swift::ICommand* command)
                    {
                        const auto renderer = g_engine.GetSystem<Renderer>();
                        const struct PushConstants
                        {
                            u32 total_draw_count;
                            u32 base_draw_offset;
                            u32 draw_buffer_index;
                            u32 indirect_command_buffer_index;
                            u32 count_buffer_index;
                        } pc
                        {
                            .total_draw_count = mesh_count,
                            .base_draw_offset = base_offset,
                            .draw_buffer_index = renderer->GetDrawDataBufferView()->GetDescriptorIndex(),
                            .indirect_command_buffer_index = indirect_cmd_view->GetDescriptorIndex(),
                            .count_buffer_index = count_view->GetDescriptorIndex(),
                        };
                        command->PushConstants(&pc, sizeof(PushConstants));
                        command->DispatchCompute((mesh_count + 63) / 64, 1, 1);
                    });
    };

    const u32 opaque_count = renderer->GetOpaqueDraws().size();
    const u32 translucent_count = renderer->GetTranslucentDraws().size();

    AddFillPass("Fill Commands Opaque", opaque_count, 0,
                m_indirect_buffer_view, m_opaque_count_buffer_views[frame_index]);
    AddFillPass("Fill Commands Translucent", translucent_count, opaque_count,
                m_indirect_buffer_view, m_translucent_count_buffer_views[frame_index]);
}
