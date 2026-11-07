#pragma once

#include <vulkan/vulkan.h>

#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>

namespace se
{
    inline void logVkResult(VkResult result, std::string_view operation) noexcept
    {
        if (result != VK_SUCCESS)
        {
            std::fprintf(stderr, "[Vulkan] %.*s failed with VkResult %d\n",
                static_cast<int>(operation.size()), operation.data(), static_cast<int>(result));
        }
    }

    inline void checkVkResult(VkResult result, std::string_view operation)
    {
        if (result == VK_SUCCESS)
        {
            return;
        }

        const std::string message = std::string{operation} + " failed with VkResult " +
            std::to_string(static_cast<int>(result));
        std::cerr << "[Vulkan] " << message << std::endl;
        throw std::runtime_error(message);
    }
}
