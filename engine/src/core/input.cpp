#include "GLFW/glfw3.h"
#include "core/engine.hpp"
#include "core/device.hpp"
#include "core/input.hpp"

using namespace Apex;

enum EKeyAction
{
    Release = 0,
    Press = 1,
    None = 2
};

constexpr int nr_keys = 350;
bool keys_down[nr_keys];
bool prev_keys_down[nr_keys];
EKeyAction keys_action[nr_keys];

constexpr int nr_mousebuttons = 8;
bool mousebuttons_down[nr_mousebuttons];
bool prev_mousebuttons_down[nr_mousebuttons];
EKeyAction mousebuttons_action[nr_mousebuttons];

constexpr int max_nr_gamepads = 4;
bool gamepad_connected[max_nr_gamepads];
GLFWgamepadstate gamepad_state[max_nr_gamepads];
GLFWgamepadstate prev_gamepad_state[max_nr_gamepads];

glm::vec2 mousepos;
float mousewheel = 0;

void CursorPositionCallback(GLFWwindow*, const double xPos, double yPos)
{
    mousepos.x = static_cast<float>(xPos);
    mousepos.y = static_cast<float>(yPos);
}

void ScrollCallback(GLFWwindow*, double, const double yOffset) { mousewheel += (float)yOffset; }

void KeyCallback(GLFWwindow*, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) keys_action[key] = static_cast<EKeyAction>(action);
}

void MouseButtonCallback(GLFWwindow*, int button, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) mousebuttons_action[button] = static_cast<EKeyAction>(action);
}

void Input::Init()
{
    auto* window = static_cast<GLFWwindow*>(g_engine.GetSystem<Device>()->GetWindow());

    glfwSetCursorPosCallback(window, CursorPositionCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    Input::Update(g_engine.GetDeltaTime());
}

void Input::Update(float)
{
    for (int i = 0; i < nr_keys; ++i)
    {
        prev_keys_down[i] = keys_down[i];

        if (keys_action[i] == Press)
            keys_down[i] = true;
        else if (keys_action[i] == Release)
            keys_down[i] = false;

        keys_action[i] = None;
    }

    for (int i = 0; i < nr_mousebuttons; ++i)
    {
        prev_mousebuttons_down[i] = mousebuttons_down[i];

        if (mousebuttons_action[i] == Press)
            mousebuttons_down[i] = true;
        else if (mousebuttons_action[i] == Release)
            mousebuttons_down[i] = false;

        mousebuttons_action[i] = None;
    }

    for (int i = 0; i < max_nr_gamepads; ++i)
    {
        prev_gamepad_state[i] = gamepad_state[i];

        if (glfwJoystickPresent(i) && glfwJoystickIsGamepad(i))
            gamepad_connected[i] = static_cast<bool>(glfwGetGamepadState(i, &gamepad_state[i]));
    }
}

void Input::Shutdown() {}

bool Input::IsGamepadAvailable(int gamepadID) const { return gamepad_connected[gamepadID]; }

float Input::GetGamepadAxis(int gamepadID, EGamepadAxis axis) const
{
    if (!IsGamepadAvailable(gamepadID)) return 0.0;

    int a = static_cast<int>(axis);
    assert(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);
    return gamepad_state[gamepadID].axes[a];
}

float Input::GetGamepadAxisPrevious(int gamepadID, EGamepadAxis axis) const
{
    if (!IsGamepadAvailable(gamepadID)) return 0.0;

    int a = static_cast<int>(axis);
    assert(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);
    return prev_gamepad_state[gamepadID].axes[a];
}

bool Input::GetGamepadButton(int gamepadID, EGamepadButton button) const
{
    if (!IsGamepadAvailable(gamepadID)) return false;

    int b = static_cast<int>(button);
    assert(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::GetGamepadButtonOnce(int gamepadID, EGamepadButton button) const
{
    if (!IsGamepadAvailable(gamepadID)) return false;

    int b = static_cast<int>(button);

    assert(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return !static_cast<bool>(prev_gamepad_state[gamepadID].buttons[b]) &&
           static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::IsMouseAvailable() const { return true; }

bool Input::GetMouseButton(EMouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b];
}

bool Input::GetMouseButtonOnce(EMouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b] && !prev_mousebuttons_down[b];
}

glm::vec2 Input::GetMousePosition() const { return mousepos; }

float Input::GetMouseWheel() const { return mousewheel; }

bool Input::IsKeyboardAvailable() const { return true; }

bool Input::GetKeyboardKey(EKeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k];
}

bool Input::GetKeyboardKeyOnce(EKeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k] && !prev_keys_down[k];
}

// NOLINTEND(readability-convert-member-functions-to-static, misc-unused-parameters)