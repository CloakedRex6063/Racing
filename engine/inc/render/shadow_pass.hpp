#pragma once
#include "render_pass.hpp"

namespace Apex
{
    class ShadowPass : public IRenderPass
    {
    public:
        static constexpr u32 k_shadow_map_extent = 4096;
        static constexpr u32 k_shadow_cascades = 1;
        static constexpr u32 k_cbv_stride = 256;
        APEX_NO_MOVE(ShadowPass);
        APEX_NO_COPY(ShadowPass);
        ShadowPass();
        ~ShadowPass() override;
        void Execute() override;

    private:
        void GenerateCascades();
        static std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
        std::vector<glm::mat4> GetLightSpaceMatrices() const;
        static glm::mat4 GetLightSpaceMatrix(float near_plane, float far_plane);
        Swift::IShader* m_shadow_shader = nullptr;
        Swift::ITexture* m_shadow_texture = nullptr;
        std::array<Swift::ITextureView*, k_shadow_cascades> m_shadow_texture_dsvs;
        std::array<Swift::ITextureView*, k_shadow_cascades> m_shadow_texture_srvs;
        Swift::IBuffer* m_pass_constant_buffer = nullptr;

        struct ShadowData
        {
            glm::mat4 shadow_light_matrix;
            float cascade_plane_distance;
            u32 shadow_map_index;
            glm::vec2 padding;
        };

        std::array<ShadowData, k_shadow_cascades> m_shadow_data;
        std::array<float, k_shadow_cascades> m_shadow_cascade_levels;
    };
}
