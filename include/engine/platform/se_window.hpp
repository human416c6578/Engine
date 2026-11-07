#pragma once

#include <vulkan/vulkan.h>

#include <string>

struct GLFWwindow;

namespace se
{
    class SEWindow
    {
    public:
        SEWindow(uint32_t width, uint32_t height, std::string name);
        ~SEWindow();

        SEWindow(const SEWindow &) = delete;
        SEWindow &operator=(const SEWindow &) = delete;
        SEWindow(SEWindow &&) = delete;
        SEWindow &operator=(SEWindow &&) = delete;

        bool shouldClose() const;
        VkExtent2D getExtent() const;
        VkExtent2D getFramebufferExtent() const;
        bool wasWindowResized() const;
        void resetWindowResizedFlag();
        void setTitle(const std::string& title);
        GLFWwindow *getGLFWwindow() const { return m_window; }

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    private:
        GLFWwindow *m_window{nullptr};

        uint32_t m_width{0};
        uint32_t m_height{0};
        std::string m_name;

        bool m_framebufferResized{false};

        void initWindow();
        static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    };
}
