#include "se_offscreen_renderer.hpp"


#include <array>

namespace se 
{

    SEOffscreenRenderer::~SEOffscreenRenderer() {
        vkDestroyFramebuffer(seDevice.device(), framebuffer, nullptr);
        vkDestroyRenderPass(seDevice.device(), renderPass, nullptr);

        vkDestroyImageView(seDevice.device(), colorImageView, nullptr);
        vkDestroyImage(seDevice.device(), colorImage, nullptr);
        vkFreeMemory(seDevice.device(), colorImageMemory, nullptr);

        vkDestroyImageView(seDevice.device(), depthImageView, nullptr);
        vkDestroyImage(seDevice.device(), depthImage, nullptr);
        vkFreeMemory(seDevice.device(), depthImageMemory, nullptr);

        vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);

        freeCommandBuffers();
    }

    void SEOffscreenRenderer::resize(uint32_t newWidth, uint32_t newHeight)
    {
        // Avoid unnecessary work
        if (width == newWidth && height == newHeight) return;

        width = newWidth;
        height = newHeight;

        vkDestroyFramebuffer(seDevice.device(), framebuffer, nullptr);
        vkDestroyRenderPass(seDevice.device(), renderPass, nullptr);

        vkDestroyImageView(seDevice.device(), colorImageView, nullptr);
        vkDestroyImage(seDevice.device(), colorImage, nullptr);
        vkFreeMemory(seDevice.device(), colorImageMemory, nullptr);

        vkDestroyImageView(seDevice.device(), depthImageView, nullptr);
        vkDestroyImage(seDevice.device(), depthImage, nullptr);
        vkFreeMemory(seDevice.device(), depthImageMemory, nullptr);

        vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);

        freeCommandBuffers();

        createRenderPass();
        createOffscreenImageDepth();
        createOffscreenImageColor();
        createFramebuffer();
        createCommandBuffers();
        createStagingBuffer();
    }

    void SEOffscreenRenderer::setImageFormat(VkFormat format)
    {
        // Avoid unnecessary work
        if (colorFormat == format) return;

        colorFormat = format;

        vkDestroyFramebuffer(seDevice.device(), framebuffer, nullptr);
        vkDestroyRenderPass(seDevice.device(), renderPass, nullptr);

        vkDestroyImageView(seDevice.device(), colorImageView, nullptr);
        vkDestroyImage(seDevice.device(), colorImage, nullptr);
        vkFreeMemory(seDevice.device(), colorImageMemory, nullptr);

        vkDestroyImageView(seDevice.device(), depthImageView, nullptr);
        vkDestroyImage(seDevice.device(), depthImage, nullptr);
        vkFreeMemory(seDevice.device(), depthImageMemory, nullptr);

        vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);

        freeCommandBuffers();

        createRenderPass();
        createOffscreenImageDepth();
        createOffscreenImageColor();
        createFramebuffer();
        createCommandBuffers();
        createStagingBuffer();
    }

    void SEOffscreenRenderer::createOffscreenImageDepth() {
        VkFormat depthFormat = seDevice.findDepthFormat();
        
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
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        seDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImage,
            depthImageMemory);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(seDevice.device(), &viewInfo, nullptr, &depthImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }
    }

    void SEOffscreenRenderer::createOffscreenImageColor() {
        // Create image
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
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        seDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            colorImage,
            colorImageMemory);
        
        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = colorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = colorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Correct aspect mask for color image
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(seDevice.device(), &viewInfo, nullptr, &colorImageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
    }


    void SEOffscreenRenderer::createRenderPass() {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = seDevice.findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.dstSubpass = 0;
        dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

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

    void SEOffscreenRenderer::createFramebuffer() {
        std::array<VkImageView, 2> attachments = { colorImageView, depthImageView };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(seDevice.device(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create offscreen framebuffer!");
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
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = seDevice.getCommandPool();
        allocInfo.commandBufferCount = 1;

        commandBuffers.resize(1);
        if (vkAllocateCommandBuffers(seDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void SEOffscreenRenderer::freeCommandBuffers()
    {
        vkFreeCommandBuffers(
            seDevice.device(),
            seDevice.getCommandPool(),
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data());
    }


} // namespace se
