#include "engine/renderer/se_offscreen_renderer.hpp"
#include "engine/vulkan/se_device.hpp"
#include "engine/vulkan/se_vk_result.hpp"

#include <array>

namespace se 
{

    SEOffscreenRenderer::~SEOffscreenRenderer()
    {
        destroyOwnedHandles();
    }

    void SEOffscreenRenderer::destroyOwnedHandles() noexcept
    {
        for (VkFramebuffer framebuffer : framebuffers)
        {
            if (framebuffer != VK_NULL_HANDLE)
                vkDestroyFramebuffer(seDevice.device(), framebuffer, nullptr);
        }
        framebuffers.clear();

        if (renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(seDevice.device(), renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }

        for (std::size_t i = 0; i < colorImages.size(); ++i)
        {
            if (i < colorImageViews.size() && colorImageViews[i] != VK_NULL_HANDLE)
                vkDestroyImageView(seDevice.device(), colorImageViews[i], nullptr);
            if (colorImages[i] != VK_NULL_HANDLE)
                vkDestroyImage(seDevice.device(), colorImages[i], nullptr);
            if (i < colorImageMemory.size() && colorImageMemory[i] != VK_NULL_HANDLE)
                vkFreeMemory(seDevice.device(), colorImageMemory[i], nullptr);
        }
        colorImageViews.clear();
        colorImages.clear();
        colorImageMemory.clear();

        for (std::size_t i = 0; i < depthImages.size(); ++i)
        {
            if (i < depthImageViews.size() && depthImageViews[i] != VK_NULL_HANDLE)
                vkDestroyImageView(seDevice.device(), depthImageViews[i], nullptr);
            if (depthImages[i] != VK_NULL_HANDLE)
                vkDestroyImage(seDevice.device(), depthImages[i], nullptr);
            if (i < depthImageMemory.size() && depthImageMemory[i] != VK_NULL_HANDLE)
                vkFreeMemory(seDevice.device(), depthImageMemory[i], nullptr);
        }
        depthImageViews.clear();
        depthImages.clear();
        depthImageMemory.clear();

        if (stagingBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
            stagingBuffer = VK_NULL_HANDLE;
        }
        if (stagingBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);
            stagingBufferMemory = VK_NULL_HANDLE;
        }
    }

    void SEOffscreenRenderer::resize(uint32_t newWidth, uint32_t newHeight)
    {
        if (width == newWidth && height == newHeight) return;

        checkVkResult(vkDeviceWaitIdle(seDevice.device()), "vkDeviceWaitIdle(offscreen resize)");
        destroyOwnedHandles();

        width = newWidth;
        height = newHeight;

        createOffscreenImageDepth();
		createOffscreenImageColor();
		createRenderPass();
		createFramebuffers();
		createStagingBuffer();
    }

    void SEOffscreenRenderer::setImageFormat(VkFormat format)
    {
        if (colorFormat == format) return;

        checkVkResult(vkDeviceWaitIdle(seDevice.device()), "vkDeviceWaitIdle(offscreen format change)");
        destroyOwnedHandles();
        colorFormat = format;

        createOffscreenImageDepth();
        createOffscreenImageColor();
        createRenderPass();
        createFramebuffers();
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

        checkVkResult(vkCreateImageView(seDevice.device(), &viewInfo, nullptr, &depthImageViews[i]), "vkCreateImageView");
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

        checkVkResult(vkCreateImageView(seDevice.device(), &viewInfo, nullptr, &colorImageViews[i]), "vkCreateImageView");
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

        VkSubpassDependency dependencyEnd{};
        dependencyEnd.srcSubpass    = 0;
        dependencyEnd.dstSubpass    = VK_SUBPASS_EXTERNAL;
        dependencyEnd.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencyEnd.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencyEnd.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencyEnd.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkSubpassDependency dependencyStart{};
        dependencyStart.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependencyStart.dstSubpass    = 0;
        dependencyStart.srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencyStart.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencyStart.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencyStart.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


        std::array<VkSubpassDependency, 2> dependencies = { dependencyStart, dependencyEnd };


        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies   = dependencies.data();

        checkVkResult(vkCreateRenderPass(seDevice.device(), &renderPassInfo, nullptr, &renderPass), "vkCreateRenderPass");
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

            checkVkResult(vkCreateFramebuffer(seDevice.device(), &framebufferInfo, nullptr, &framebuffers[i]), "vkCreateFramebuffer");
        }
    }

    void SEOffscreenRenderer::createStagingBuffer() {
        VkDeviceSize bufferSize = width * height * 4; // Assuming 4 bytes per pixel (RGBA8)

        seDevice.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory);
    }


    void SEOffscreenRenderer::beginRenderPass(VkCommandBuffer commandBuffer, std::size_t frameIndex)
    {

        // Begin the offscreen render pass (this is similar to beginSwapChainRenderPass)
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = getFramebuffer(frameIndex);
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
        vkCmdEndRenderPass(commandBuffer);
    }


} // namespace se
