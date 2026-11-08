#pragma once
struct GLFWwindow;

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

        bool isKeyPressed(int key) const;
        bool isMouseButtonPressed(int button) const;
        void getMousePosition(double &x, double &y) const;
        void setCursorCaptured(bool captured) const;

    private:
        GLFWwindow *m_window;
    };
}

