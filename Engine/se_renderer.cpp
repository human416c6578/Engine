#include "se_renderer.hpp"

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

    SERenderer::~SERenderer() { freeCommandBuffers(); }

    void SERenderer::recreateSwapChain()
    {
        auto extent = seWindow.getExtent();
        while (extent.width == 0 || extent.height == 0)
        {
            extent = seWindow.getExtent();
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(seDevice.device());

        if (seSwapChain == nullptr)
        {
            seSwapChain = std::make_unique<SESwapChain>(seDevice, extent);
        }
        else
        {
            std::shared_ptr<SESwapChain> oldSwapChain = std::move(seSwapChain);
            seSwapChain = std::make_unique<SESwapChain>(seDevice, extent, oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*seSwapChain.get()))
            {
                throw std::runtime_error("Swap chain image(or depth) format has changed!");
            }
        }
    }

    void SERenderer::createCommandBuffers()
    {
        commandBuffers.resize(SESwapChain::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = seDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(seDevice.device(), &allocInfo, commandBuffers.data()) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void SERenderer::freeCommandBuffers()
    {
        vkFreeCommandBuffers(
            seDevice.device(),
            seDevice.getCommandPool(),
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data());
        commandBuffers.clear();
    }

    VkCommandBuffer SERenderer::beginFrame()
    {
        assert(!isFrameStarted && "Can't call beginFrame while already in progress");

        auto result = seSwapChain->acquireNextImage(&currentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return nullptr;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        isFrameStarted = true;

        auto commandBuffer = getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
        return commandBuffer;
    }

    void SERenderer::endFrame()
    {
        assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
        auto commandBuffer = getCurrentCommandBuffer();
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }

        auto result = seSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            seWindow.wasWindowResized())
        {
            seWindow.resetWindowResizedFlag();
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        isFrameStarted = false;
    }

    void SERenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
    {
        assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
        assert(
            commandBuffer == getCurrentCommandBuffer() &&
            "Can't begin render pass on command buffer from a different frame");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = seSwapChain->getRenderPass();
        renderPassInfo.framebuffer = seSwapChain->getFrameBuffer(currentImageIndex);

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = seSwapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(seSwapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(seSwapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{ {0, 0}, seSwapChain->getSwapChainExtent() };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void SERenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
    {
        assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
        assert(
            commandBuffer == getCurrentCommandBuffer() &&
            "Can't end render pass on command buffer from a different frame");
        vkCmdEndRenderPass(commandBuffer);
    }

    VkCommandBuffer SERenderer::beginOffscreenFrame() {
        assert(!isOffscreenFrameStarted && "Can't call beginOffscreenFrame while already in progress");

        // Get the current offscreen command buffer
        auto commandBuffer = offscreenRenderer->getCommandBuffer();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording offscreen command buffer!");
        }

        isOffscreenFrameStarted = true;
        return commandBuffer;
    }
    
    void SERenderer::endOffscreenFrame() {
        assert(isOffscreenFrameStarted && "Can't call endOffscreenFrame while frame is not in progress");
        auto commandBuffer = offscreenRenderer->getCommandBuffer();
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record offscreen command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;


        VkResult result = vkQueueSubmit(seDevice.presentQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to submit offscreen command buffer!");
        }

        isOffscreenFrameStarted = false;
    }


    void SERenderer::beginOffscreenRenderPass(VkCommandBuffer commandBuffer)
    {
        assert(isOffscreenFrameStarted && "Can't call beginOffscreenRenderPass if frame is not in progress");

        // Begin the offscreen render pass (this is similar to beginSwapChainRenderPass)
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = offscreenRenderer->getRenderPass();
        renderPassBeginInfo.framebuffer = offscreenRenderer->getFramebuffer();
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { offscreenRenderer->getWidth(), offscreenRenderer->getHeight() };

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f }; // Clear color for offscreen
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Set up viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(offscreenRenderer->getWidth());
        viewport.height = static_cast<float>(offscreenRenderer->getHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{ {0, 0}, {offscreenRenderer->getWidth(), offscreenRenderer->getHeight()} };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void SERenderer::endOffscreenRenderPass(VkCommandBuffer commandBuffer)
    {
        assert(isOffscreenFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
        assert(
            commandBuffer == offscreenRenderer->getCommandBuffer() &&
            "Can't end render pass on command buffer from a different frame");
        vkCmdEndRenderPass(commandBuffer);
    }

}