#include "engine/platform/se_window.hpp"
#include "engine/core/se_logger.hpp"
#include "engine/vulkan/se_vk_result.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <stdexcept>
#include <iostream>
#include <utility>

namespace se
{
    SEWindow::SEWindow(uint32_t width, uint32_t height, std::string name)
        : m_width{width}, m_height{height}, m_name{std::move(name)}
    {
        initWindow();
    }

    SEWindow::~SEWindow()
    {
        if (m_window != nullptr)
        {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
    }

    bool SEWindow::shouldClose() const
    {
        return glfwWindowShouldClose(m_window) != 0;
    }

    VkExtent2D SEWindow::getExtent() const
    {
        return {m_width, m_height};
    }

    VkExtent2D SEWindow::getFramebufferExtent() const
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    bool SEWindow::wasWindowResized() const
    {
        return m_framebufferResized;
    }

    void SEWindow::resetWindowResizedFlag()
    {
        m_framebufferResized = false;
    }

    void SEWindow::setTitle(const std::string& title)
    {
        glfwSetWindowTitle(m_window, title.c_str());
    }

    void SEWindow::initWindow()
    {
        glfwSetErrorCallback([](int code, const char* message) {
            log::error("GLFW", "[%d] %s", code, message);
        });
        if (glfwInit() != GLFW_TRUE)
        {
            throw std::runtime_error("glfwInit failed");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
        if (m_window == nullptr)
        {
            glfwTerminate();
            throw std::runtime_error("glfwCreateWindow failed");
        }
        glfwSetWindowUserPointer(m_window, this);

        glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    }

    void SEWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface)
    {
        checkVkResult(glfwCreateWindowSurface(instance, m_window, nullptr, surface),
            "glfwCreateWindowSurface");
    }

    void SEWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto Window = reinterpret_cast<SEWindow *>(glfwGetWindowUserPointer(window));
        if (Window == nullptr)
        {
            return;
        }
        Window->m_framebufferResized = true;
        Window->m_width = width;
        Window->m_height = height;
    }
};
