#include "core/app.hpp"
#include "core/components.hpp"
#include "core/renderer.hpp"
#include "core/scene.hpp"
#include "managers/camera_manager.hpp"
#include "managers/resource_manager.hpp"
#include "managers/scene_manager.hpp"

class Racing final : public Apex::Application
{
public:
    void Init() override
    {
        Apex::g_engine.GetSystem<Apex::SceneManager>()->SetActiveScene(std::make_shared<Apex::Scene>());
        const auto& resource_manager = Apex::g_engine.GetSystem<Apex::ResourceManager>();
        auto result = resource_manager->InstantiateModel(
            "assets/redbull.glb");
        assert(result.has_value() && "Failed to load model");
        auto dir_light = Apex::ECS::CreateEntity("Directional Light");
        Apex::ECS::AddComponent<Apex::Component::DirectionalLight>(dir_light);

        auto sky_result = Apex::g_engine.GetSystem<Apex::Renderer>()->SetSkybox("assets/skybox/sky.dds");
        assert(sky_result.has_value() && "Failed to load skybox");
    }

    void Update() override
    {
    }

    void Shutdown() override
    {
    }
};

Apex::Application* CreateApplication() { return new Racing(); }
