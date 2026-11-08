#pragma once

#include <vulkan/vulkan.h>

// std lib headers
#include <memory>
#include <vector>

namespace se
{
  class SEDevice;

  class SESwapChain
  {
  public:

    SESwapChain(SEDevice &deviceRef, VkExtent2D windowExtent);
    SESwapChain(
        SEDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<SESwapChain> previous);

    ~SESwapChain();

    SESwapChain(const SESwapChain &) = delete;
    SESwapChain &operator=(const SESwapChain &) = delete;
    SESwapChain(SESwapChain&&) = delete;
    SESwapChain& operator=(SESwapChain&&) = delete;

	size_t getCurrentFrame() const { return currentFrame; }
    VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return renderPass; }
    VkImageView getImageView(int index) { return swapChainImageViews[index]; }
    size_t imageCount() const { return swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    VkDescriptorSetLayout getDescriptorSetLayout() { return descriptorSetLayout; }
    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }

    float extentAspectRatio()
    {
      return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t *imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

    bool compareSwapFormats(const SESwapChain &swapChain) const
    {
      return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
             swapChain.swapChainImageFormat == swapChainImageFormat;
    }

  private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createFramebuffers();
    void createSyncObjects();
    void destroyOwnedHandles() noexcept;

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkFormat swapChainImageFormat{VK_FORMAT_UNDEFINED};
    VkFormat swapChainDepthFormat{VK_FORMAT_UNDEFINED};
    VkExtent2D swapChainExtent{};

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass{VK_NULL_HANDLE};
    VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    SEDevice &device;
    VkExtent2D windowExtent{};

    VkSwapchainKHR swapChain{VK_NULL_HANDLE};
    std::shared_ptr<SESwapChain> oldSwapChain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
  };

}
