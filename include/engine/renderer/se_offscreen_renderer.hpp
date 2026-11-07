#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <vector>

namespace se
{
    class SEDevice;
    class SEOffscreenRenderer
    {
    public:
        explicit SEOffscreenRenderer(SEDevice& device)
            : seDevice(device)
        {
            try
            {
                createOffscreenImageDepth();
                createOffscreenImageColor();
                createRenderPass();
                createFramebuffers();
                createStagingBuffer();
            }
            catch (...)
            {
                destroyOwnedHandles();
                throw;
            }
        }

        ~SEOffscreenRenderer();

        SEOffscreenRenderer(const SEOffscreenRenderer&) = delete;
        SEOffscreenRenderer& operator=(const SEOffscreenRenderer&) = delete;
        SEOffscreenRenderer(SEOffscreenRenderer&&) = delete;
        SEOffscreenRenderer& operator=(SEOffscreenRenderer&&) = delete;

        void resize(uint32_t newWidth, uint32_t newHeight);
        void setImageFormat(VkFormat format);

        VkRenderPass getRenderPass() const { return renderPass; }

        VkFramebuffer getFramebuffer(std::size_t frameIndex) const
        {
            return framebuffers[frameIndex];
        }

        VkImage getDepthImage(std::size_t frameIndex) const { return depthImages[frameIndex]; }
        VkImage getColorImage(std::size_t frameIndex) const { return colorImages[frameIndex]; }
        VkImageView getDepthImageView(std::size_t frameIndex) const { return depthImageViews[frameIndex]; }
        VkImageView getColorImageView(std::size_t frameIndex) const { return colorImageViews[frameIndex]; }

        uint32_t getWidth() const { return width; }
        uint32_t getHeight() const { return height; }

        VkBuffer getStagingBuffer() const { return stagingBuffer; }
        VkDeviceMemory getStagingBufferMemory() const { return stagingBufferMemory; }

        void beginRenderPass(VkCommandBuffer commandBuffer, std::size_t frameIndex);
        void endRenderPass(VkCommandBuffer commandBuffer);

    private:
        void createOffscreenImageDepth();
        void createOffscreenImageColor();
        void createRenderPass();
        void createFramebuffers();
        void createStagingBuffer();
        void destroyOwnedHandles() noexcept;

        SEDevice& seDevice;

        uint32_t width{ 512 };
        uint32_t height{ 512 };
        VkFormat colorFormat{ VK_FORMAT_R8G8B8A8_SRGB };

        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> framebuffers;

        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemory;
        std::vector<VkImageView> depthImageViews;

        std::vector<VkImage> colorImages;
        std::vector<VkDeviceMemory> colorImageMemory;
        std::vector<VkImageView> colorImageViews;

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    };
}
