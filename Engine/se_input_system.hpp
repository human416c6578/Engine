#pragma once
#include <GLFW/glfw3.h>

namespace se
{
    class SEInputSystem
    {
    public:
        static void initialize(GLFWwindow* window) {
            s_window = window;
        }

        static bool isKeyPressed(int key) {
            return glfwGetKey(s_window, key) == GLFW_PRESS;
        }

        static bool isMouseButtonPressed(int button) {
            return glfwGetMouseButton(s_window, button) == GLFW_PRESS;
        }

        static void getMousePosition(double& x, double& y) {
            glfwGetCursorPos(s_window, &x, &y);
        }

    private:
        static GLFWwindow* s_window;
    };
}

