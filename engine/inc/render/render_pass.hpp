#pragma once

namespace Apex
{
    class IRenderPass
    {
    public:
        APEX_CONSTRUCT(IRenderPass);
        APEX_DESTRUCT(IRenderPass);
        APEX_NO_COPY(IRenderPass);
        APEX_NO_MOVE(IRenderPass);
        virtual void Execute() = 0;
    };
}