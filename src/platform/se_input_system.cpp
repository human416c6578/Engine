#include "engine/platform/se_input_system.hpp"

#include <GLFW/glfw3.h>

namespace se
{
    bool SEInputSystem::isKeyPressed(int key) const
    {
        return glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    bool SEInputSystem::isMouseButtonPressed(int button) const
    {
        return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
    }

    void SEInputSystem::getMousePosition(double &x, double &y) const
    {
        glfwGetCursorPos(m_window, &x, &y);
    }

    void SEInputSystem::setCursorCaptured(bool captured) const
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}
