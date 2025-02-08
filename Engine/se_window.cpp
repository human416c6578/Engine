#include "se_window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <stdexcept>

namespace se
{
    SEWindow::SEWindow(uint32_t width, uint32_t height, std::string name) : m_width{width}, m_height{height}, m_name{name}
    {
        initWindow();
    }

    SEWindow::~SEWindow()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void SEWindow::initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);

        glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    }

    void SEWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface)
    {
        if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface");
        }
    }

    void SEWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto Window = reinterpret_cast<SEWindow *>(glfwGetWindowUserPointer(window));
        Window->m_framebufferResized = true;
        Window->m_width = width;
        Window->m_height = height;
    }
};