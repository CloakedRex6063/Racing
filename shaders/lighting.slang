#define PI 3.14159265359

struct PointLight
{
    float3 position;
    float intensity = 1.0f;
    float3 color = float3(1.0f);
    float range = 100.0f;
};

struct DirectionalLight
{
    float3 direction;
    float intensity = 1.0f;
    float3 color = float3(1.0f);
    int cast_shadows = false;
};

float IorToF0(float ior)
{
    float f = (ior - 1.0) / (ior + 1.0);
    return f * f;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 GetIBLRadianceLambertian(float3 n, float3 v, float roughness, float3 diffuseColor, float3 F0, SamplerCube diffuse_texture, Sampler2D lut_texture)
{
    float NoV = saturate(dot(n, v));
    float2 brdfSamplePoint = clamp(float2(NoV, roughness), float2(0.0, 0.0), float2(1.0, 1.0));
    float2 f_ab = lut_texture.Sample(brdfSamplePoint).rg;

    float3 irradiance = diffuse_texture.Sample(n).rgb;

    // see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
    // Roughness dependent fresnel, from Fdez-Aguera

    float3 Fr = max(float3(1.0 - roughness), F0) - F0;
    float3 k_S = F0 + Fr * pow(1.0 - NoV, 5.0);
    float3 FssEss = k_S * f_ab.x + f_ab.y;  // <--- GGX / specular light contribution

    // Multiple scattering, from Fdez-Aguera
    float Ems = (1.0 - (f_ab.x + f_ab.y));
    float3 F_avg = (F0 + (1.0 - F0) / 21.0);
    float3 FmsEms = Ems * FssEss * F_avg / (1.0 - F_avg * Ems);
    float3 k_D = diffuseColor *
               (1.0 - FssEss +
                FmsEms);  // we use +FmsEms as indicated by the formula in the blog post (might be a typo in the implementation)

    return (FmsEms + k_D) * irradiance;
}

float3 GetIBLRadianceGGX(float3 n, float3 v, float roughness, float3 F0, SamplerCube specular_texture, Sampler2D lut_texture)
{
    float NoV = saturate(dot(n, v));
    float lod = roughness * float(6);
    float3 reflection = normalize(reflect(-v, n));

    float2 brdfSamplePoint = clamp(float2(NoV, roughness), float2(0.0, 0.0), float2(1.0, 1.0));
    float2 f_ab = lut_texture.Sample(brdfSamplePoint).rg;
    float4 specularSample = specular_texture.SampleLevel(reflection, lod);

    float3 specularLight = specularSample.rgb;

    // see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
    // Roughness dependent fresnel, from Fdez-Aguera
    float3 Fr = max(float3(1.0 - roughness), F0) - F0;
    float3 k_S = F0 + Fr * pow(1.0 - NoV, 5.0);
    float3 FssEss = k_S * f_ab.x + f_ab.y;
    return specularLight * FssEss;
}

float3 CalculateIBL(float3 N,
                    float3 V,
                    float3 diffuse,
                    float roughness,
                    float3 F0,
                    float clearcoat,
                    float clearcoatRoughness,
                    float3 clearcoatN,  // may differ from N if clearcoat normal map is used
                    SamplerCube diffuse_texture,
                    SamplerCube specular_texture,
                    Sampler2D lut_texture)
{
    float3 specular = GetIBLRadianceGGX(N, V, roughness, F0, specular_texture, lut_texture);
    float3 diffuseIBL = GetIBLRadianceLambertian(N, V, roughness, diffuse, F0, diffuse_texture, lut_texture);
    float3 baseIBL = diffuseIBL + specular;

    // Clearcoat layer: IOR 1.5 → F0 = ((1.5-1)/(1.5+1))^2 = 0.04
    const float3 clearcoatF0 = float3(0.04, 0.04, 0.04);
    float NdotV = saturate(dot(clearcoatN, V));

    // Fresnel for clearcoat (Schlick)
    float3 Fc = clearcoatF0 + (1.0 - clearcoatF0) * pow(1.0 - NdotV, 5.0);

    // Clearcoat specular IBL uses its own normal and roughness
    float3 clearcoatSpecular = GetIBLRadianceGGX(clearcoatN, V, clearcoatRoughness, clearcoatF0, specular_texture, lut_texture);

    // Layer the clearcoat on top of the base:
    // base is attenuated by (1 - clearcoat * Fc), clearcoat adds on top
    return baseIBL * (1.0 - clearcoat * Fc) + clearcoatSpecular * clearcoat * Fc;
}

float3 CalculateIBL(float3 N,
                    float3 V,
                    float3 diffuse,
                    float roughness,
                    float3 F0,
                    float clearcoat,
                    float clearcoatRoughness,
                    SamplerCube diffuse_texture,
                    SamplerCube specular_texture,
                    Sampler2D lut_texture)
{
    float3 specular = GetIBLRadianceGGX(N, V, roughness, F0, specular_texture, lut_texture);
    float3 diffuseIBL = GetIBLRadianceLambertian(N, V, roughness, diffuse, F0, diffuse_texture, lut_texture);
    float3 baseIBL = diffuseIBL + specular;

    // Clearcoat layer: IOR 1.5 → F0 = ((1.5-1)/(1.5+1))^2 = 0.04
    const float3 clearcoatF0 = float3(0.04, 0.04, 0.04);
    float NdotV = saturate(dot(N, V));

    // Fresnel for clearcoat (Schlick)
    float3 Fc = clearcoatF0 + (1.0 - clearcoatF0) * pow(1.0 - NdotV, 5.0);

    // Clearcoat uses the same N as the base — no clearcoat normal mapping
    float3 clearcoatSpecular = GetIBLRadianceGGX(N, V, clearcoatRoughness, clearcoatF0, specular_texture, lut_texture);

    return baseIBL * (1.0 - clearcoat * Fc) + clearcoatSpecular * clearcoat * Fc;
}

float3 ApplyLight(float3 L,
                  float3 radiance,
                  float3 F0,
                  float3 N,
                  float3 V,
                  float3 clearcoatN,
                  float roughness,
                  float metallic,
                  float3 albedo,
                  float clearcoat,
                  float clearcoatRoughness)
{
    float3 H = normalize(V + L);

    // Cook-Torrance BRDF
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = D * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    float3 kD = (float3(1.0) - F) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    float3 baseLayer = (kD * albedo / PI + specular) * radiance * NdotL;

    // Clearcoat BRDF using clearcoat normal
    const float3 clearcoatF0 = float3(0.04, 0.04, 0.04);
    float3 ccH = normalize(V + L);

    float ccD = DistributionGGX(clearcoatN, ccH, clearcoatRoughness);
    float ccG = GeometrySmith(clearcoatN, V, L, clearcoatRoughness);
    float3 ccF = FresnelSchlick(max(dot(ccH, V), 0.0), clearcoatF0);

    float ccNdotL = max(dot(clearcoatN, L), 0.0);
    float ccDenominator = 4.0 * max(dot(clearcoatN, V), 0.0) * ccNdotL + 0.0001;
    float3 clearcoatSpecular = (ccD * ccG * ccF) / ccDenominator;

    // Fresnel attenuation on base, clearcoat on top
    float3 Fc = ccF * clearcoat;
    return baseLayer * (1.0 - Fc) + clearcoatSpecular * clearcoat * radiance * ccNdotL;
}

float3 ApplyLight(float3 L,
                  float3 radiance,
                  float3 F0,
                  float3 N,
                  float3 V,
                  float roughness,
                  float metallic,
                  float3 albedo,
                  float clearcoat,
                  float clearcoatRoughness)
{
    float3 H = normalize(V + L);

    // Cook-Torrance BRDF
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = D * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    float3 kD = (float3(1.0) - F) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    float3 baseLayer = (kD * albedo / PI + specular) * radiance * NdotL;

    // Clearcoat BRDF reusing base N
    const float3 clearcoatF0 = float3(0.04, 0.04, 0.04);

    float ccD = DistributionGGX(N, H, clearcoatRoughness);
    float ccG = GeometrySmith(N, V, L, clearcoatRoughness);
    float3 ccF = FresnelSchlick(max(dot(H, V), 0.0), clearcoatF0);

    float ccDenominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    float3 clearcoatSpecular = (ccD * ccG * ccF) / ccDenominator;

    float3 Fc = ccF * clearcoat;
    return baseLayer * (1.0 - Fc) + clearcoatSpecular * clearcoat * radiance * NdotL;
}

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float ComputeExposure(float EV100)
{
    return 1.0 / (pow(2.0, EV100) * 1.2);
}
