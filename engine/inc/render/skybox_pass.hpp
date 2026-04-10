#pragma once
#include "render_pass.hpp"
#include "managers/resource_manager.hpp"

namespace Apex
{
    class SkyboxPass : public IRenderPass
    {
    public:
        APEX_NO_MOVE(SkyboxPass);
        APEX_NO_COPY(SkyboxPass);
        SkyboxPass();
        ~SkyboxPass() override;
        void Execute() override;
        void SetSkybox(const Texture& skybox_texture);

    private:
        Swift::IShader* m_skybox_shader = nullptr;
        Swift::ITexture* m_skybox_texture = nullptr;
        Swift::ITextureView* m_skybox_texture_view = nullptr;
    };
}
