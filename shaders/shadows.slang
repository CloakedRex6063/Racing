#include "common.slang"
#include "lighting.slang"

struct PushConstant
{
    uint meshlet_offset;
    uint transform_index;
    uint material_index;
};

ConstantBuffer<PushConstant> PushConstants : register(b0);

struct PassConstant
{
    uint cascade_index;
}
ConstantBuffer<PassConstant> PassConstants : register(b2);

struct ShadowData
{
    float4x4 shadow_matrix;
    float cascade_plane_distance;
    uint shadow_map_index;
    float2 padding;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 world_pos : POSITION;
};

[numthreads(128, 1, 1)]
[outputtopology("triangle")]
[shader("mesh")]
void mesh_main(uint gid: SV_GroupID,
               uint gtid: SV_GroupThreadID,
               OutputVertices<VertexOut, 64> verts,
               OutputIndices<uint3, 126> prims)
{
    var position_buffer = DescriptorHandle<StructuredBuffer<uint2>>(GlobalConstants.position_buffer_index);
    var transform_buffer = DescriptorHandle<StructuredBuffer<Transform>>(GlobalConstants.transform_buffer_index);
    var mesh_triangle_buffer = DescriptorHandle<StructuredBuffer<uint>>(GlobalConstants.triangle_buffer_index);
    var meshlet_buffer = DescriptorHandle<StructuredBuffer<Meshlet>>(GlobalConstants.meshlet_buffer_index);
    var shadow_buffer = DescriptorHandle<StructuredBuffer<ShadowData>>(GlobalConstants.shadow_buffer_index);
    var gpu_transform = transform_buffer[PushConstants.transform_index];
    float4x4 transform = gpu_transform.transform;

    Meshlet meshlet = meshlet_buffer[PushConstants.meshlet_offset + gid];
    SetMeshOutputCounts(meshlet.vertex_count, meshlet.triangle_count);

    if (gtid < meshlet.triangle_count)
    {
        uint packed = mesh_triangle_buffer[meshlet.triangle_offset + gtid];
        uint idx0 = (packed >> 0) & 0xFF;
        uint idx1 = (packed >> 8) & 0xFF;
        uint idx2 = (packed >> 16) & 0xFF;
        prims[gtid] = uint3(idx0, idx1, idx2);
    }

    if (gtid < meshlet.vertex_count)
    {
        uint vertex_index = meshlet.vertex_offset + gtid;
        uint2 quant_pos = position_buffer[vertex_index];
        uint xy = asuint(quant_pos.x);
        float3 position = float3(f16tof32(xy & 0xFFFF), f16tof32(xy >> 16), f16tof32(asuint(quant_pos.y) & 0xFFFF));
        float4x4 final_transform = mul(shadow_buffer[PassConstants.cascade_index].shadow_matrix, transform);
        verts[gtid].position = mul(final_transform, float4(position, 1.0));
    }
}

[shader("pixel")]
void pixel_main(VertexOut input) {}
