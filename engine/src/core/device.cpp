#include "core/device.hpp"
#include "core/engine.hpp"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "tools/log.hpp"

using namespace Apex;

void ErrorCallback(int, const char* description) { static_cast<void>(std::fputs(description, stderr)); }

Device::Device() {}

Device::~Device() = default;

void Device::Init()
{
    if (!glfwInit())
    {
        Log::Critical("GLFW init failed");
    }

    Log::Info("GLFW version {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(1280, 720, "Firestorm", nullptr, nullptr);

    if (!m_window)
    {
        Log::Critical("GLFW window could not be created");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowSizeCallback(m_window,
                  [](GLFWwindow*, int w, int h)
                  {
                      const auto& device = g_engine.GetSystem<Device>();
                      for (auto& callback : device->m_resize_callbacks)
                      {
                          callback({w, h});
                      }
                  });
}

void Device::Update(float) { glfwPollEvents(); }

void Device::Shutdown() { glfwTerminate(); }

void Device::LockMouse(const bool toggle) const
{
    const auto value = (toggle ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    glfwSetInputMode(m_window, GLFW_CURSOR, value);
}

bool Device::ShouldClose() const { return glfwWindowShouldClose(m_window); }

void* Device::GetWindow() const { return m_window; }

void* Device::GetNativeWindow() const
{
    return glfwGetWin32Window(m_window);
}

glm::vec2 Device::GetWindowSize() const
{
    int width = 0, height = 0;
    glfwGetWindowSize(m_window, &width, &height);
    return { width, height };
}
