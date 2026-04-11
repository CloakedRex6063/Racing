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
    float4x4 inv_transform = gpu_transform.inv_transform;

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

        Vertex vertex = vertex_buffer[vertex_index];
        uint uvPacked = vertex.uv;
        float2 uv;
        uv.x = f16tof32(uvPacked & 0xFFFF);
        uv.y = f16tof32(uvPacked >> 16);

        uint nPacked = vertex.normal;
        float3 normal;
        normal.x = (int((nPacked >> 20) & 0x3FF) << 22) >> 22;
        normal.y = (int((nPacked >> 10) & 0x3FF) << 22) >> 22;
        normal.z = (int((nPacked >> 0) & 0x3FF) << 22) >> 22;
        normal = normalize(normal / 511.0);

        uint tPacked = vertex.tangent;
        float3 tangent;
        tangent.x = (int((tPacked >> 20) & 0x3FF) << 22) >> 22;
        tangent.y = (int((tPacked >> 10) & 0x3FF) << 22) >> 22;
        tangent.z = (int((tPacked >> 0) & 0x3FF) << 22) >> 22;
        tangent = tangent / 511.0;

        verts[gtid].normal = mul(float3x3(inv_transform), normal);
        verts[gtid].uv = uv;
        verts[gtid].tangent = float4(mul(float3x3(inv_transform), tangent), vertex.bitangent_sign);

        uint2 quant_pos = position_buffer[vertex_index];
        uint xy = asuint(quant_pos.x);
        float3 position = float3(f16tof32(xy & 0xFFFF), f16tof32(xy >> 16), f16tof32(asuint(quant_pos.y) & 0xFFFF));
        float4 world_pos = mul(transform, float4(position, 1.0));
        verts[gtid].position = mul(GlobalConstants.view_proj, world_pos);
        verts[gtid].world_pos = world_pos.xyz;
    }
}

[shader("pixel")]
float4 pixel_main(VertexOut input) : SV_Target
{
    var material_buffer = DescriptorHandle<StructuredBuffer<Material>>(GlobalConstants.material_buffer_index);
    Material material = material_buffer[PushConstants.material_index];

    float2 ior_opacity = UnpackHalf2(material.ior_opacity);

    float4 albedo = float4(UnpackSnorm(material.albedo), ior_opacity.y);
    var albedo_texture = DescriptorHandle<Sampler2D>(uint2(material.albedo_index, GlobalConstants.bilinear_sampler_index));
    float4 tex = albedo_texture.Sample(input.uv);
    float3 texColor = pow(tex.rgb, 2.2);
    float alpha = tex.a;
    albedo.rgb *= texColor;
    albedo.a *= alpha;

    float2 metal_rough = UnpackHalf2(material.metal_rough);
    float metallic = metal_rough.x;
    float roughness = metal_rough.y;
    var metal_rough_texture =
        DescriptorHandle<Sampler2D>(uint2(material.metal_rough_index, GlobalConstants.bilinear_sampler_index));
    float4 mr = metal_rough_texture.Sample(input.uv);
    roughness *= mr.g;
    metallic *= mr.b;

    float3 emissive = UnpackSnorm(material.emissive);
    var emissive_texture = DescriptorHandle<Sampler2D>(uint2(material.emissive_index, GlobalConstants.bilinear_sampler_index));
    emissive *= pow(emissive_texture.Sample(input.uv).rgb, 2.2);

    float3 ao = 1.0;
    var occlusion_texture =
        DescriptorHandle<Sampler2D>(uint2(material.occlusion_index, GlobalConstants.bilinear_sampler_index));
    ao *= occlusion_texture.Sample(input.uv).rgb;

    var normal_texture = DescriptorHandle<Sampler2D>(uint2(material.normal_index, GlobalConstants.bilinear_sampler_index));
    float3 tangentNormal = normal_texture.Sample(input.uv).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;

    float3 T = normalize(input.tangent.xyz);
    float3 N = normalize(input.normal);
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T) * input.tangent.w;
    float3x3 TBN = { T.x, B.x, N.x, T.y, B.y, N.y, T.z, B.z, N.z };
    N = mul(TBN, tangentNormal);

    var clearcoat_normal_texture =
        DescriptorHandle<Sampler2D>(uint2(material.clearcoat_normal_index, GlobalConstants.bilinear_sampler_index));
    float3 clearcoatTangentNormal = clearcoat_normal_texture.Sample(input.uv).rgb;
    clearcoatTangentNormal = clearcoatTangentNormal * 2.0 - 1.0;

    float3 clearcoatN = mul(clearcoatTangentNormal, TBN);

    float3 V = normalize(GlobalConstants.cam_pos - input.world_pos);

    float3 F0 = IorToF0(ior_opacity.x);
    F0 = lerp(F0, albedo.rgb, metallic);

    float2 cc = UnpackHalf2(material.clear_coat);
    float3 dndx = ddx(N);
    float3 dndy = ddy(N);
    float variance = dot(dndx, dndx) + dot(dndy, dndy);
    float kernelRoughness = min(2.0 * variance, 0.25);

    roughness = sqrt(saturate(roughness * roughness + kernelRoughness));
    cc.y = sqrt(saturate(cc.y * cc.y + kernelRoughness));

    var diffuse_ibl_texture =
        DescriptorHandle<SamplerCube>(uint2(GlobalConstants.diffuse_ibl_index, GlobalConstants.bilinear_sampler_index));
    var specular_ibl_texture =
        DescriptorHandle<SamplerCube>(uint2(GlobalConstants.specular_ibl_index, GlobalConstants.bilinear_sampler_index));
    var lut_texture = DescriptorHandle<Sampler2D>(uint2(GlobalConstants.lut_ibl_index, GlobalConstants.bilinear_sampler_index));
    var dir_light_buffer = DescriptorHandle<StructuredBuffer<DirectionalLight>>(GlobalConstants.dir_buffer_index);
    float3 lo = 0.0;
    float3 ibl = 0.0;
    if (HasFlag(material.flags, FLAG_USE_CLEARCOAT_NORMAL_MAP))
    {
        for (int i = 0; i < GlobalConstants.dir_light_count; i++)
        {
            var dir_light = dir_light_buffer[i];
            float3 radiance = dir_light.color * dir_light.intensity;
            lo += ApplyLight(normalize(-dir_light.direction),
                             radiance,
                             F0,
                             N,
                             V,
                             clearcoatN,
                             roughness * roughness,
                             metallic,
                             albedo.rgb,
                             cc.x,
                             cc.y);
            ibl = CalculateIBL(N,
                               V,
                               albedo.rgb * (1.0 - metallic),
                               roughness,
                               F0,
                               cc.x,
                               cc.y,
                               diffuse_ibl_texture,
                               specular_ibl_texture,
                               lut_texture);
        }
    }
    else
    {
        for (int i = 0; i < GlobalConstants.dir_light_count; i++)
        {
            var dir_light = dir_light_buffer[i];
            float3 radiance = dir_light.color * dir_light.intensity;
            lo += ApplyLight(normalize(dir_light.direction),
                             radiance,
                             F0,
                             N,
                             V,
                             roughness * roughness,
                             metallic,
                             albedo.rgb,
                             cc.x,
                             cc.y);
            ibl = CalculateIBL(N,
                               V,
                               albedo.rgb * (1.0 - metallic),
                               roughness,
                               F0,
                               cc.x,
                               cc.y,
                               clearcoatN,
                               diffuse_ibl_texture,
                               specular_ibl_texture,
                               lut_texture);
        }
    }

    float3 color = ibl * ao * 0.5 + lo + emissive;
    color = ACESFilm(color);
    color = pow(color, 1.0 / 2.2);

    return float4(color, albedo.a);
}
