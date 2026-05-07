#include "engine/renderer/se_offscreen_renderer.hpp"


#include <array>

namespace se 
{

    SEOffscreenRenderer::~SEOffscreenRenderer()
    {
        for (size_t i = 0; i < framebuffers.size(); i++)
        {
            vkDestroyFramebuffer(seDevice.device(), framebuffers[i], nullptr);
        }

        vkDestroyRenderPass(seDevice.device(), renderPass, nullptr);

        for (size_t i = 0; i < colorImageViews.size(); i++)
        {
            vkDestroyImageView(seDevice.device(), colorImageViews[i], nullptr);
            vkDestroyImage(seDevice.device(), colorImages[i], nullptr);
            vkFreeMemory(seDevice.device(), colorImageMemory[i], nullptr);
        }

        for (size_t i = 0; i < depthImageViews.size(); i++)
        {
            vkDestroyImageView(seDevice.device(), depthImageViews[i], nullptr);
            vkDestroyImage(seDevice.device(), depthImages[i], nullptr);
            vkFreeMemory(seDevice.device(), depthImageMemory[i], nullptr);
        }

        vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);

        freeCommandBuffers();
    }

    void SEOffscreenRenderer::resize(uint32_t newWidth, uint32_t newHeight)
    {
        if (width == newWidth && height == newHeight) return;

        vkDeviceWaitIdle(seDevice.device());

        width = newWidth;
        height = newHeight;

        for (size_t i = 0; i < framebuffers.size(); i++)
        {
            vkDestroyFramebuffer(seDevice.device(), framebuffers[i], nullptr);
        }
        framebuffers.clear();

        vkDestroyRenderPass(seDevice.device(), renderPass, nullptr);

        for (size_t i = 0; i < colorImageViews.size(); i++)
        {
            vkDestroyImageView(seDevice.device(), colorImageViews[i], nullptr);
            vkDestroyImage(seDevice.device(), colorImages[i], nullptr);
            vkFreeMemory(seDevice.device(), colorImageMemory[i], nullptr);
        }

        for (size_t i = 0; i < depthImageViews.size(); i++)
        {
            vkDestroyImageView(seDevice.device(), depthImageViews[i], nullptr);
            vkDestroyImage(seDevice.device(), depthImages[i], nullptr);
            vkFreeMemory(seDevice.device(), depthImageMemory[i], nullptr);
        }

        colorImageViews.clear();
        colorImages.clear();
        colorImageMemory.clear();

        depthImageViews.clear();
        depthImages.clear();
        depthImageMemory.clear();

        vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);

        freeCommandBuffers();

        createOffscreenImageDepth();
		createOffscreenImageColor();
		createRenderPass();
		createFramebuffers();
		createCommandBuffers();
		createStagingBuffer();
    }

    void SEOffscreenRenderer::setImageFormat(VkFormat format)
    {
        if (colorFormat == format) return;

        colorFormat = format;

        for (size_t i = 0; i < framebuffers.size(); i++)
            vkDestroyFramebuffer(seDevice.device(), framebuffers[i], nullptr);

        vkDestroyRenderPass(seDevice.device(), renderPass, nullptr);

        for (size_t i = 0; i < colorImages.size(); i++)
        {
            vkDestroyImageView(seDevice.device(), colorImageViews[i], nullptr);
            vkDestroyImage(seDevice.device(), colorImages[i], nullptr);
            vkFreeMemory(seDevice.device(), colorImageMemory[i], nullptr);
        }

        for (size_t i = 0; i < colorImages.size(); i++)
        {
            vkDestroyImageView(seDevice.device(), depthImageViews[i], nullptr);
            vkDestroyImage(seDevice.device(), depthImages[i], nullptr);
            vkFreeMemory(seDevice.device(), depthImageMemory[i], nullptr);
        }

        vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);

        freeCommandBuffers();

        createRenderPass();
        createOffscreenImageDepth();
        createOffscreenImageColor();
        createFramebuffers();
        createCommandBuffers();
        createStagingBuffer();
    }

    void SEOffscreenRenderer::createOffscreenImageDepth() {
    VkFormat depthFormat = seDevice.findDepthFormat();

    depthImages.resize(MAX_FRAMES_IN_FLIGHT);
    depthImageMemory.resize(MAX_FRAMES_IN_FLIGHT);
    depthImageViews.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        seDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImages[i],
            depthImageMemory[i]);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(seDevice.device(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create depth image view!");
        }
    }
}

    void SEOffscreenRenderer::createOffscreenImageColor() {
    colorImages.resize(MAX_FRAMES_IN_FLIGHT);
    colorImageMemory.resize(MAX_FRAMES_IN_FLIGHT);
    colorImageViews.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = colorFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        seDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            colorImages[i],
            colorImageMemory[i]);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = colorImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = colorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(seDevice.device(), &viewInfo, nullptr, &colorImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create color image view!");
        }
    }
}


    void SEOffscreenRenderer::createRenderPass() {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = seDevice.findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = colorFormat;
        
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = 0;
        dependency.dstSubpass = VK_SUBPASS_EXTERNAL;

        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        dependency.srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        dependency.dstStageMask =
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        dependency.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(seDevice.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void SEOffscreenRenderer::createFramebuffers()
    {
        framebuffers.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            std::array<VkImageView, 2> attachments = {
                colorImageViews[i],
                depthImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 2;
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(seDevice.device(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create offscreen framebuffer!");
            }
        }
    }

    void SEOffscreenRenderer::createStagingBuffer() {
        VkDeviceSize bufferSize = width * height * 4; // Assuming 4 bytes per pixel (RGBA8)

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(seDevice.device(), &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create staging buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(seDevice.device(), stagingBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = seDevice.findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(seDevice.device(), &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate staging buffer memory!");
        }

        vkBindBufferMemory(seDevice.device(), stagingBuffer, stagingBufferMemory, 0);
    }


    void SEOffscreenRenderer::createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = seDevice.getCommandPool();
        allocInfo.commandBufferCount = 1;

        for (auto& commandBuffer : commandBuffers)
        {
            if (vkAllocateCommandBuffers(seDevice.device(), &allocInfo, &commandBuffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate offscreen command buffer!");
            }
        }
    }

    void SEOffscreenRenderer::freeCommandBuffers()
    {
        for (auto& commandBuffer : commandBuffers)
        {
            vkFreeCommandBuffers(
                seDevice.device(),
                seDevice.getCommandPool(),
                1,
                &commandBuffer);
        }

        commandBuffers.clear();
    }

    VkCommandBuffer SEOffscreenRenderer::beginFrame() {
        assert(!frameStarted && "Can't call beginOffscreenFrame while already in progress");
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        // Get the current offscreen command buffer
        auto commandBuffer = getCommandBuffer();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording offscreen command buffer!");
        }

        frameStarted = true;
        
        return commandBuffer;
    }
    
    void SEOffscreenRenderer::endFrame() {
        assert(frameStarted && "Can't call endOffscreenFrame while frame is not in progress");
        auto commandBuffer = getCommandBuffer();
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

        frameStarted = false;
    }


    void SEOffscreenRenderer::beginRenderPass(VkCommandBuffer commandBuffer)
    {
        assert(frameStarted && "Can't call beginOffscreenRenderPass if frame is not in progress");

        // Begin the offscreen render pass (this is similar to beginSwapChainRenderPass)
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = getFramebuffer(currentFrame);
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { width, height };

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
        viewport.width = static_cast<float>(width);
        viewport.height = static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{ {0, 0}, {width, height} };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void SEOffscreenRenderer::endRenderPass(VkCommandBuffer commandBuffer)
    {
        assert(frameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
        assert(
            commandBuffer == getCommandBuffer() &&
            "Can't end render pass on command buffer from a different frame");
        vkCmdEndRenderPass(commandBuffer);
    }


} // namespace se
