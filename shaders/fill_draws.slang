#include "common.slang"

struct DrawData
{
    uint meshlet_count;
    uint meshlet_offset;
    uint transform_index;
    uint material_index;
}

struct PushConstant
{
    uint total_draw_count;
    uint base_draw_offset;
    uint draw_buffer_index;
    uint transform_buffer_index;
    uint meshlet_buffer_index;
    uint indirect_command_buffer_index;
    uint count_buffer_index;
};

ConstantBuffer<PushConstant> PushConstants : register(b0);

[numthreads(32, 1, 1)]
[shader("compute")]
void main(uint dtid: SV_DispatchThreadID)
{
    if (dtid >= PushConstants.total_draw_count)
        return;

    var draw_buffer = DescriptorHandle<StructuredBuffer<DrawData>>(PushConstants.draw_buffer_index);
    // var transform_buffer = DescriptorHandle<StructuredBuffer<float4x4>>(PushConstants.transform_buffer_index);
    var meshlet_buffer = DescriptorHandle<StructuredBuffer<Meshlet>>(PushConstants.meshlet_buffer_index);
    var indirect_command_buffer =
        DescriptorHandle<RWStructuredBuffer<IndirectCommand>>(PushConstants.indirect_command_buffer_index);
    var count_buffer = DescriptorHandle<RWStructuredBuffer<uint>>(PushConstants.count_buffer_index);

    DrawData draw = draw_buffer[dtid + PushConstants.base_draw_offset];

    uint slot;
    InterlockedAdd(count_buffer[0], 1, slot);

    IndirectCommand cmd;
    cmd.meshlet_offset = draw.meshlet_offset;
    cmd.transform_index = draw.transform_index;
    cmd.material_index = draw.material_index;
    cmd.dispatch_x = draw.meshlet_count;
    cmd.dispatch_y = 1;
    cmd.dispatch_z = 1;

    indirect_command_buffer[PushConstants.base_draw_offset + slot] = cmd;
}
