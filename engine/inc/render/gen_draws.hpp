#pragma once
#include "render_pass.hpp"

namespace Apex
{
    struct IndirectMeshCommand
    {
        u32 meshlet_count;
        u32 meshlet_offset;
        u32 transform_index;
        u32 material_index;
        u32 dispatch_x;
        u32 dispatch_y;
        u32 dispatch_z;
    };
    class GenDrawsPass : public IRenderPass
    {
    public:
        APEX_NO_MOVE(GenDrawsPass);
        APEX_NO_COPY(GenDrawsPass);
        GenDrawsPass();
        ~GenDrawsPass() override;
        void Execute() override;

        Swift::IBuffer* GetIndirectCommandBuffer() const { return m_indirect_buffer; }
        Swift::IBuffer* GetCountBuffer() const { return m_count_buffer; }
        Swift::ICommandSignature* GetMeshCommandSignature() const { return m_mesh_command_signature; }

    private:
        Swift::IShader* m_fill_shader = nullptr;
        Swift::IBuffer* m_indirect_buffer = nullptr;
        Swift::IBuffer* m_count_buffer = nullptr;
        Swift::IBufferView* m_indirect_buffer_view = nullptr;
        std::array<Swift::IBufferView*, 3> m_opaque_count_buffer_views;
        std::array<Swift::IBufferView*, 3> m_translucent_count_buffer_views;
        Swift::ICommandSignature* m_mesh_command_signature = nullptr;
    };
}
