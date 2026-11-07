#pragma once

#include "engine/core/se_frame_config.hpp"

#include <vulkan/vulkan.h>

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
