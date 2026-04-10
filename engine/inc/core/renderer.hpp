#pragma once
#include "components.hpp"
#include "system.hpp"
#include "render/render_pass.hpp"

namespace Apex
{
    struct GPUPointLight
    {
        glm::vec3 position;
        float intensity = 1.0f;
        glm::vec3 color = glm::vec3(1.0f);
        float range = 100.0f;
    };

    struct GPUDirectionalLight
    {
        glm::vec3 direction = {-1, -1, -1};
        float intensity = 1.0f;
        glm::vec3 color = glm::vec3(1.0f);
        int cast_shadows = false;
    };

    struct DrawData
    {
        u32 meshlet_count;
        u32 meshlet_offset;
        u32 transform_index;
        u32 material_index;
    };

    class Renderer : public System
    {
    public:
        static constexpr u32 k_max_draw_count = 100'000;
        static constexpr u32 k_max_vertex_count = 100'000'000;
        static constexpr u32 k_max_triangle_count = 100'000'000;
        static constexpr u32 k_max_meshlet_count = 10'000'000;
        static constexpr u32 k_max_material_count = 10'000;
        static constexpr u32 k_max_point_light_count = 1'000;
        static constexpr u32 k_max_dir_light_count = 10;

        enum class Error
        {
            eFailedToLoadSkybox,
            eFailedToLoadDiffuseIBL,
            eFailedToLoadSpecularIBL,
            eFailedToLoadLut,
        };

        void Init() override;
        void Update(float) override;
        void Shutdown() override;

        [[nodiscard]] Swift::IContext* GetContext() const { return m_context; }
        [[nodiscard]] Swift::IBuffer* GetPositionBuffer() const { return m_position_buffer; }
        [[nodiscard]] Swift::IBuffer* GetVertexBuffer() const { return m_vertex_buffer; }
        [[nodiscard]] Swift::IBuffer* GetTriangleBuffer() const { return m_triangle_buffer; }
        [[nodiscard]] Swift::IBuffer* GetMeshletBuffer() const { return m_meshlet_buffer; }
        [[nodiscard]] Swift::IBuffer* GetTransformBuffer() const { return m_transform_buffer; }
        [[nodiscard]] Swift::IBuffer* GetMaterialBuffer() const { return m_material_buffer; }
        [[nodiscard]] Swift::IBuffer* GetDrawDataBuffer() const { return m_draw_buffer; }
        [[nodiscard]] Swift::IBuffer* GetDirLightBuffer() const { return m_dir_light_buffer; }
        [[nodiscard]] Swift::IBuffer* GetPointLightBuffer() const { return m_point_light_buffer; }
        [[nodiscard]] Swift::IBuffer* GetShadowBuffer() const { return m_shadow_buffer; }
        [[nodiscard]] Swift::IBufferView* GetPositionBufferView() const { return m_position_buffer_view; }
        [[nodiscard]] Swift::IBufferView* GetVertexBufferView() const { return m_vertex_buffer_view; }
        [[nodiscard]] Swift::IBufferView* GetTriangleBufferView() const { return m_triangle_buffer_view; }
        [[nodiscard]] Swift::IBufferView* GetMeshletBufferView() const { return m_meshlet_buffer_view; }
        [[nodiscard]] Swift::IBufferView* GetTransformBufferView() const { return m_transform_buffer_view; }
        [[nodiscard]] Swift::IBufferView* GetMaterialBufferView() const { return m_material_buffer_view; }
        [[nodiscard]] Swift::IBufferView* GetDrawDataBufferView() const { return m_draw_buffer_view; }
        [[nodiscard]] Swift::RG::RenderGraph& GetRenderGraph() { return m_render_graph; }
        [[nodiscard]] Swift::ITextureView* GetDepthTargetView() const { return m_depth_target_view; }

        u32 AddTransform(const glm::mat4& transform);
        void UpdateTransform(u32 transform_index, const glm::mat4& transform);
        u32 AddDirLight(const GPUDirectionalLight& dir_light);
        u32 AddPointLight(const GPUPointLight& point_light);

        void MarkDrawBufferDirty() { m_draw_buffer_dirty = true; }
        std::span<const DrawData> GetOpaqueDraws() const { return m_opaque_draws; }
        std::span<const DrawData> GetTranslucentDraws() const { return m_translucent_draws; }

        std::expected<void, Error> SetSkybox(const fs::path& path);

        template <typename T>
        void AddRenderPass()
        {
            m_render_passes.emplace_back(std::make_unique<T>());
        }

        template <typename T>
        T& GetRenderPass()
        {
            for (auto& pass : m_render_passes)
            {
                if (auto it = std::dynamic_pointer_cast<T>(pass))
                {
                    return *it;
                }
            }
            assert("Pass not found");
        }

    private:
        void BuildBuffers();
        void BuildSamplers();
        void UpdateGlobalConstantBuffer() const;
        void UpdateTransformBuffer();

        struct GPUTransform
        {
            glm::mat4 transform;
            glm::mat4 inv_transform;
        };

        struct GlobalConstants
        {
            glm::mat4 view;
            glm::mat4 proj;
            glm::mat4 view_proj;

            glm::vec3 cam_pos;
            u32 position_buffer_index;

            u32 vertex_buffer_index;
            u32 triangle_buffer_index;
            u32 meshlet_buffer_index;
            u32 transform_buffer_index;

            u32 material_buffer_index;
            u32 shadow_buffer_index;
            u32 point_buffer_index;
            u32 point_light_count;

            u32 dir_buffer_index;
            u32 dir_light_count;
            u32 diffuse_ibl_index;
            u32 specular_ibl_index;

            u32 lut_ibl_index;
            u32 bilinear_sampler_index;
            u32 nearest_sampler_index;
        };

        struct ShadowData
        {
            glm::mat4 shadow_matrix;
            float split_depth;
            u32 shadow_map_index;
            glm::vec2 padding;
        };

        Swift::IContext* m_context = nullptr;
        Swift::IBuffer* m_position_buffer = nullptr;
        Swift::IBufferView* m_position_buffer_view = nullptr;
        Swift::IBuffer* m_vertex_buffer = nullptr;
        Swift::IBufferView* m_vertex_buffer_view = nullptr;
        Swift::IBuffer* m_triangle_buffer = nullptr;
        Swift::IBufferView* m_triangle_buffer_view = nullptr;
        Swift::IBuffer* m_meshlet_buffer = nullptr;
        Swift::IBufferView* m_meshlet_buffer_view = nullptr;

        Swift::IBuffer* m_transform_buffer = nullptr;
        Swift::IBufferView* m_transform_buffer_view = nullptr;
        std::unordered_map<u32, GPUTransform> m_pending_transforms;
        std::vector<u32> m_free_transforms;
        u32 m_transform_count = 0;

        Swift::IBuffer* m_material_buffer = nullptr;
        Swift::IBufferView* m_material_buffer_view = nullptr;

        Swift::IBuffer* m_draw_buffer = nullptr;
        Swift::IBufferView* m_draw_buffer_view = nullptr;
        std::vector<DrawData> m_opaque_draws;
        std::vector<DrawData> m_translucent_draws;
        bool m_draw_buffer_dirty = false;

        Swift::IBuffer* m_shadow_buffer = nullptr;
        Swift::IBufferView* m_shadow_buffer_view = nullptr;

        Swift::IBuffer* m_point_light_buffer = nullptr;
        Swift::IBufferView* m_point_light_buffer_view = nullptr;
        std::vector<u32> m_free_point_lights;
        u32 m_point_light_count = 0;
        Swift::IBuffer* m_dir_light_buffer = nullptr;
        Swift::IBufferView* m_dir_light_buffer_view = nullptr;
        std::vector<u32> m_free_dir_lights;
        u32 m_dir_light_count = 0;

        Swift::IBuffer* m_global_constant_buffer = nullptr;

        Swift::ITexture* m_depth_target = nullptr;
        Swift::ITextureView* m_depth_target_view = nullptr;

        Swift::ISampler* m_bilinear_sampler = nullptr;
        Swift::ISampler* m_nearest_sampler = nullptr;

        Swift::ITexture* m_diffuse_ibl_texture = nullptr;
        Swift::ITextureView* m_diffuse_ibl_texture_view = nullptr;

        Swift::ITexture* m_specular_ibl_texture = nullptr;
        Swift::ITextureView* m_specular_ibl_texture_view = nullptr;

        Swift::ITexture* m_lut_texture = nullptr;
        Swift::ITextureView* m_lut_texture_view = nullptr;

        Swift::RG::RenderGraph m_render_graph;

        std::vector<Ref<IRenderPass>> m_render_passes;

        static constexpr u32 k_constant_buffer_aligned_size = (sizeof(GlobalConstants) + 255) & ~255;
    };
}