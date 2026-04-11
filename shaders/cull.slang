struct BoundingSphere
{
    float3 center;
    float radius;
}

struct CullData
{
    float3 center;
    float radius;
    float3 cone_apex;
    uint cone_packed;
};

float3 GetScale(float4x4 m)
{
    return float3(length(float3(m[0][0], m[1][0], m[2][0])),
                  length(float3(m[0][1], m[1][1], m[2][1])),
                  length(float3(m[0][2], m[1][2], m[2][2])));
}

bool IsVisibleAfterFrustumCull(float4 planes[6], float4x4 transform, CullData cull_data)
{
    var world_center = mul(transform, float4(cull_data.center, 1.0));
    float3 scale = GetScale(transform);
    float radius = cull_data.radius * max(scale.x, max(scale.y, scale.z));

    for (int i = 0; i < 6; i++)
    {
        float dist = dot(planes[i].xyz, world_center.xyz) + planes[i].w;
        if (dist < -radius)
            return false;
    }
    return true;
}
