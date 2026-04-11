#include "common.slang"
#include "lighting.slang"
#include "cull.slang"

struct PushConstant
{
    uint meshlet_count;
    uint meshlet_offset;
    uint transform_index;
    uint material_index;
};

ConstantBuffer<PushConstant> PushConstants : register(b0);

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 world_pos : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

struct ASPayload
{
    uint visible_meshlet_indices[AS_GROUP_SIZE];
};

[numthreads(AS_GROUP_SIZE, 1, 1)]
[shader("amplification")]
void ampl_main(uint gtid: SV_GroupThreadID, uint dtid: SV_DispatchThreadID)
{
    bool in_bounds = dtid < PushConstants.meshlet_count;

    var transform_buffer = DescriptorHandle<StructuredBuffer<Transform>>(GlobalConstants.transform_buffer_index);
    var cull_buffer = DescriptorHandle<StructuredBuffer<CullData>>(GlobalConstants.cull_data_buffer_index);
    var transform = transform_buffer[PushConstants.transform_index].transform;
    var cull_data = cull_buffer[PushConstants.meshlet_offset + dtid];
    bool visible = in_bounds && IsVisibleAfterFrustumCull(GlobalConstants.frustum, transform, cull_data);

    ASPayload payload;
    uint visible_mask = WaveActiveBallot(visible).x;
    uint compact_slot = countbits(visible_mask & ((1u << gtid) - 1u));
    if (visible)
        payload.visible_meshlet_indices[compact_slot] = dtid;

    DispatchMesh(countbits(visible_mask), 1, 1, payload);
}

[numthreads(128, 1, 1)]
[outputtopology("triangle")]
[shader("mesh")]
void mesh_main(uint gid: SV_GroupID,
               uint gtid: SV_GroupThreadID,
               in payload ASPayload payload,
               OutputVertices<VertexOut, 64> verts,
               OutputIndices<uint3, 126> prims)
{
    var position_buffer = DescriptorHandle<StructuredBuffer<uint2>>(GlobalConstants.position_buffer_index);
    var vertex_buffer = DescriptorHandle<StructuredBuffer<Vertex>>(GlobalConstants.vertex_buffer_index);
    var meshlet_buffer = DescriptorHandle<StructuredBuffer<Meshlet>>(GlobalConstants.meshlet_buffer_index);
    var transform_buffer = DescriptorHandle<StructuredBuffer<Transform>>(GlobalConstants.transform_buffer_index);
    var mesh_triangle_buffer = DescriptorHandle<StructuredBuffer<uint>>(GlobalConstants.triangle_buffer_index);
    var gpu_transform = transform_buffer[PushConstants.transform_index];
    float4x4 transform = gpu_transform.transform;

    uint local_index = payload.visible_meshlet_indices[gid];
    uint meshlet_index = PushConstants.meshlet_offset + local_index;
    Meshlet meshlet = meshlet_buffer[meshlet_index];
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
        float4 world_pos = mul(transform, float4(position, 1.0));
        verts[gtid].position = mul(GlobalConstants.view_proj, world_pos);
        verts[gtid].world_pos = world_pos.xyz;
    }
}

[shader("pixel")]
void pixel_main()
{
}
