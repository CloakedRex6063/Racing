#include "core/scene.hpp"

#include "core/ecs.hpp"

void Apex::Scene::Init() { mRegistry = ECS::CreateRegistry(); }