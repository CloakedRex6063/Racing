#include "render/skybox_pass.hpp"
#include "shader_data.hpp"
#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/renderer.hpp"

Apex::SkyboxPass::SkyboxPass()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    m_skybox_shader = Swift::GraphicsShaderBuilder(context)
                      .SetRTVFormats({Swift::Format::eRGBA16F})
                      .SetDSVFormat(Swift::Format::eD32F)
                      .SetMeshShader(skybox_mesh_main_code)
                      .SetPixelShader(skybox_pixel_main_code)
                      .SetDepthTest(Swift::ComparisonFunc::eLessEqual)
                      .SetDepthTestEnable(true)
                      .SetDepthWriteEnable(true)
                      .SetCullMode(Swift::CullMode::eNone)
                      .SetPolygonMode(Swift::PolygonMode::eTriangle)
                      .SetName("Skybox Shader")
                      .Build();
}

Apex::SkyboxPass::~SkyboxPass()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    context->DestroyTexture(m_skybox_texture);
    context->DestroyTextureView(m_skybox_texture_view);
    context->DestroyShader(m_skybox_shader);
}

void Apex::SkyboxPass::Execute()
{
    const auto& renderer = g_engine.GetSystem<Renderer>();
    auto* context = renderer->GetContext();
    auto& rg = renderer->GetRenderGraph();
    const auto depth_target_view = renderer->GetDepthTargetView();
    auto window_size = g_engine.GetSystem<Device>()->GetWindowSize();

    rg.AddRenderPass("Skybox Pass", m_skybox_shader)
      .SetRenderExtents(Swift::Float2(window_size.x, window_size.y))
      .WriteRenderTarget(context->GetCurrentRenderTarget()).
      WriteDepthStencil(depth_target_view)
      .SetExecute(
          [&](Swift::ICommand* command)
          {
              const struct PushConstants
              {
                  u32 skybox_index;
              } pc{
                  .skybox_index = m_skybox_texture_view->GetDescriptorIndex(),
              };
              command->PushConstants(&pc, sizeof(PushConstants));
              command->DispatchMesh(1, 1, 1);
          });
}

void Apex::SkyboxPass::SetSkybox(const Texture& skybox_texture)
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    if (m_skybox_texture)
    {
        context->GetGraphicsQueue()->WaitIdle();
        context->DestroyTexture(m_skybox_texture);
        context->DestroyTextureView(m_skybox_texture_view);
    }
    m_skybox_texture = skybox_texture.m_texture;
    m_skybox_texture_view = skybox_texture.m_texture_view;
}
