#include "engine/renderer/se_renderer.hpp"
#include "engine/renderer/se_swap_chain.hpp"
#include <GLFW/glfw3.h>
#include "engine/vulkan/se_device.hpp"
#include "engine/platform/se_window.hpp"
#include "engine/renderer/se_offscreen_renderer.hpp"
#include "engine/vulkan/se_vk_result.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

namespace se
{
    SERenderer::SERenderer(SEWindow& window, SEDevice& device)
        : seWindow{ window }, seDevice{ device }
    {
        offscreenRenderer = std::make_unique<SEOffscreenRenderer>(seDevice);
        recreateSwapChain();
        createCommandBuffers();
    }

    VkRenderPass SERenderer::getSwapChainRenderPass() const { return seSwapChain->getRenderPass(); }

    uint32_t SERenderer::getSwapChainImageCount() const
    {
        return static_cast<uint32_t>(seSwapChain->imageCount());
    }

    float SERenderer::getAspectRatio() const { return seSwapChain->extentAspectRatio(); }

    SERenderer::~SERenderer()
    {
        freeCommandBuffers();
    }

    void SERenderer::recreateSwapChain()
    {
        auto extent = seWindow.getExtent();
        while (extent.width == 0 || extent.height == 0)
        {
            extent = seWindow.getExtent();
            glfwWaitEvents();
        }
        checkVkResult(vkDeviceWaitIdle(seDevice.device()), "vkDeviceWaitIdle(swapchain recreation)");

        if (seSwapChain == nullptr)
        {
            seSwapChain = std::make_unique<SESwapChain>(seDevice, extent);
        }
        else
        {
            std::shared_ptr<SESwapChain> oldSwapChain = std::move(seSwapChain);
            seSwapChain = std::make_unique<SESwapChain>(seDevice, extent, oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*seSwapChain))
            {
                throw std::runtime_error("Swap chain image(or depth) format has changed!");
            }
        }
    }

    void SERenderer::createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = seDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        checkVkResult(vkAllocateCommandBuffers(seDevice.device(), &allocInfo, commandBuffers.data()),
            "vkAllocateCommandBuffers(renderer)");
    }

    void SERenderer::freeCommandBuffers()
    {
        if (commandBuffers.empty())
        {
            return;
        }

        vkFreeCommandBuffers(
            seDevice.device(),
            seDevice.getCommandPool(),
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data());
        commandBuffers.clear();
    }

    std::optional<FrameContext> SERenderer::beginFrame()
    {
        assert(!isFrameStarted && "Can't call beginFrame while already in progress");

        uint32_t imageIndex = 0;
        const auto result = seSwapChain->acquireNextImage(&imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return std::nullopt;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            checkVkResult(result, "vkAcquireNextImageKHR");

        const auto frameIndex = static_cast<uint32_t>(seSwapChain->getCurrentFrame());
        const auto commandBuffer = commandBuffers[frameIndex];

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        checkVkResult(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer(renderer)");

        activeFrame = FrameContext{ frameIndex, imageIndex, commandBuffer };
        isFrameStarted = true;
        return activeFrame;
    }

    void SERenderer::endFrame(const FrameContext& frame)
    {
        assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
        assert(frame.frameIndex == activeFrame.frameIndex);
        assert(frame.swapchainImageIndex == activeFrame.swapchainImageIndex);
        assert(frame.commandBuffer == activeFrame.commandBuffer);

        checkVkResult(vkEndCommandBuffer(frame.commandBuffer), "vkEndCommandBuffer(renderer)");

        auto imageIndex = frame.swapchainImageIndex;
        const auto result = seSwapChain->submitCommandBuffers(&frame.commandBuffer, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            seWindow.wasWindowResized())
        {
            seWindow.resetWindowResizedFlag();
            isFrameStarted = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS)
        {
            isFrameStarted = false;
            activeFrame = {};
            checkVkResult(result, "vkQueuePresentKHR");
        }

        isFrameStarted = false;
        activeFrame = {};
    }

    void SERenderer::beginSwapChainRenderPass(const FrameContext& frame)
    {
        assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
        assert(frame.commandBuffer == activeFrame.commandBuffer);
        assert(frame.swapchainImageIndex == activeFrame.swapchainImageIndex);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = seSwapChain->getRenderPass();
        renderPassInfo.framebuffer = seSwapChain->getFrameBuffer(frame.swapchainImageIndex);

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = seSwapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(frame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(seSwapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(seSwapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{ {0, 0}, seSwapChain->getSwapChainExtent() };
        vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);
    }

    void SERenderer::endSwapChainRenderPass(const FrameContext& frame)
    {
        assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
        assert(frame.commandBuffer == activeFrame.commandBuffer);
        assert(frame.swapchainImageIndex == activeFrame.swapchainImageIndex);
        vkCmdEndRenderPass(frame.commandBuffer);
    }
}
