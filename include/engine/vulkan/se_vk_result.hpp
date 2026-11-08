#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>
#include <string_view>

namespace se
{
    inline void checkVkResult(VkResult result, std::string_view operation)
    {
        if (result == VK_SUCCESS)
        {
            return;
        }

        const std::string message = std::string{operation} + " failed with VkResult " +
            std::to_string(static_cast<int>(result));
        throw std::runtime_error(message);
    }
}
