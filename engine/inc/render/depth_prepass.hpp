#pragma once
#include "render_pass.hpp"

namespace Apex
{
    class DepthPrepass : public IRenderPass
    {
    public:
        APEX_NO_MOVE(DepthPrepass);
        APEX_NO_COPY(DepthPrepass);
        DepthPrepass();
        ~DepthPrepass() override;
        void Execute() override;

    private:
        Swift::IShader* m_depth_shader = nullptr;
    };
}
