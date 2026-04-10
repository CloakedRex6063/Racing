#pragma once

namespace Apex
{
    class Application
    {
    public:
        virtual ~Application() = default;

        virtual void Init() = 0;

        virtual void Update() = 0;

        virtual void Shutdown() = 0;
    };
}  // namespace Apex

extern Apex::Application* CreateApplication();

#include "core/engine.hpp"
#include "core/device.hpp"

int main()
{
    Apex::g_engine.Init();
    const auto app = CreateApplication();
    app->Init();
    while (!Apex::g_engine.GetSystem<Apex::Device>()->ShouldClose())
    {
        Apex::g_engine.BeginFrame();
        app->Update();
        Apex::g_engine.Update();
        Apex::g_engine.EndFrame();
    }
    app->Shutdown();
    delete app;
    Apex::g_engine.Shutdown();
}
