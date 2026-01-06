#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace se
{
    class SEWindow
    {
    public:
        SEWindow(uint32_t width, uint32_t height, std::string name);
        ~SEWindow();

        bool shouldClose() { return glfwWindowShouldClose(m_window); }
        VkExtent2D getExtent() { return {static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)}; }
        bool wasWindowResized() { return m_framebufferResized; }
        void resetWindowResizedFlag() { m_framebufferResized = false; }
        void setTitle(std::string title) { glfwSetWindowTitle(m_window, title.c_str());}
        GLFWwindow *getGLFWwindow() const { return m_window; }

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    private:
        GLFWwindow *m_window;

        uint32_t m_width;
        uint32_t m_height;
        std::string m_name;

        bool m_framebufferResized;

        void initWindow();
        static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    };
}