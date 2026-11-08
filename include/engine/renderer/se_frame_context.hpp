#pragma once

#include <vulkan/vulkan.h> // VkCommandBuffer is stored by value.

#include <cstdint>

namespace se
{
    struct FrameContext
    {
        uint32_t frameIndex = 0;
        uint32_t swapchainImageIndex = 0;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    };
}
