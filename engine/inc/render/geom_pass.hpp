#pragma once
#include "render_pass.hpp"

namespace Apex
{
    class GeomPass : public IRenderPass
    {
    public:
        APEX_NO_MOVE(GeomPass);
        APEX_NO_COPY(GeomPass);
        GeomPass();
        ~GeomPass() override;
        void Execute() override;

    private:
        Swift::IShader* m_opaque_shader = nullptr;
        Swift::IShader* m_translucent_shader = nullptr;
    };
}
