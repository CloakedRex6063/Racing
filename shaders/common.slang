struct GlobalConstant
{
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
    float3 cam_pos;
    uint position_buffer_index;
    uint vertex_buffer_index;
    uint triangle_buffer_index;
    uint meshlet_buffer_index;
    uint transform_buffer_index;
    uint material_buffer_index;
    uint shadow_buffer_index;
    uint point_buffer_index;
    uint point_light_count;
    uint dir_buffer_index;
    uint dir_light_count;
    uint diffuse_ibl_index;
    uint specular_ibl_index;
    uint lut_ibl_index;
    uint bilinear_sampler_index;
    uint nearest_sampler_index;
};
ConstantBuffer<GlobalConstant> GlobalConstants : register(b1);

struct Meshlet
{
    uint vertex_offset;
    uint triangle_offset;
    uint vertex_count;
    uint triangle_count;
};

struct Vertex
{
    uint normal;
    uint tangent;
    float bitangent_sign;
    uint uv;
};

struct Material
{
    uint albedo;
    uint emissive;
    uint metal_rough;
    uint ior_opacity;
    uint clear_coat;
    uint alpha_mode;
    uint flags;

    int albedo_index;
    int emissive_index;
    int metal_rough_index;
    int normal_index;
    int occlusion_index;
    int clearcoat_index;
    int clearcoat_rough_index;
    int clearcoat_normal_index;

    uint padding;
};

struct IndirectCommand
{
    uint meshlet_offset;
    uint transform_index;
    uint material_index;
    uint dispatch_x;
    uint dispatch_y;
    uint dispatch_z;
};

struct Transform
{
    float4x4 transform;
    float4x4 inv_transform;
}

float3 UnpackSnorm(uint packed)
{
    float r = float(packed & 0x3FFu) / 1023.0;
    float g = float((packed >> 10) & 0x3FFu) / 1023.0;
    float b = float((packed >> 20) & 0x3FFu) / 1023.0;

    return float3(r, g, b);
}

float2 UnpackHalf2(uint packed)
{
    return float2(f16tof32(packed & 0xFFFF), f16tof32(packed >> 16));
}

static const uint FLAG_ALPHA_TRANSPARENT = 1 << 0;
static const uint FLAG_USE_CLEARCOAT_NORMAL_MAP = 1 << 1;

bool HasFlag(uint flags, uint flag)
{
    return (flags & flag) != 0;
}

