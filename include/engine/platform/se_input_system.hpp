#pragma once

#include <GLFW/glfw3.h>
#include <array>

namespace se
{
    class SEInputSystem
    {
    public:
        explicit SEInputSystem(GLFWwindow &window) noexcept : m_window{&window} {}

        SEInputSystem(const SEInputSystem&) = delete;
        SEInputSystem& operator=(const SEInputSystem&) = delete;
        SEInputSystem(SEInputSystem&&) = delete;
        SEInputSystem& operator=(SEInputSystem&&) = delete;

        // Call once per application frame, immediately after glfwPollEvents().
        void update();

        bool isKeyPressed(int key) const; // True while held (existing behavior).
        bool isKeyJustPressed(int key) const; // Up -> down this frame; non-consuming.
        bool isMouseButtonPressed(int button) const;
        void getMousePosition(double &x, double &y) const;
        void setCursorCaptured(bool captured) const;

    private:
        GLFWwindow *m_window;
        std::array<bool, GLFW_KEY_LAST + 1> keysDown{};
        std::array<bool, GLFW_KEY_LAST + 1> keysJustPressed{};
    };
}

