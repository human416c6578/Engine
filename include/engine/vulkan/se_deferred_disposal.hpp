#pragma once

#include "engine/core/se_frame_config.hpp"
#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <type_traits>
#include <variant>

namespace se
{
    // Owning handle packets only: never retain a wrapper, callback, or CPU resource.
    struct DisposalBufferAllocation
    {
        VkBuffer buffer{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
        bool mapped{false};
    };

    struct BufferDisposal
    {
        std::array<DisposalBufferAllocation, 2> allocations{};
    };

    struct DescriptorBufferDisposal
    {
        VkDescriptorPool pool{VK_NULL_HANDLE}; // Borrow; device keeps it alive through drain.
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> sets{};
        std::array<DisposalBufferAllocation, 2 * MAX_FRAMES_IN_FLIGHT> allocations{};
    };

    struct ImageDisposal
    {
        VkDescriptorPool pool{VK_NULL_HANDLE}; // Borrow, as above.
        VkDescriptorSet set{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE};
        VkImageView view{VK_NULL_HANDLE};
        VkImage image{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
    };

    using DisposalPayload = std::variant<BufferDisposal, DescriptorBufferDisposal, ImageDisposal>;
    static_assert(std::is_nothrow_default_constructible_v<DisposalPayload>);
    static_assert(std::is_nothrow_move_constructible_v<DisposalPayload>);
    static_assert(std::is_nothrow_move_assignable_v<DisposalPayload>);
    static_assert(std::is_trivially_destructible_v<DisposalPayload>);

    // Reserve before creating Vulkan handles. Enqueue only transfers this unique owner.
    struct DeferredDisposal
    {
        DeferredDisposal() noexcept = default;
        DeferredDisposal(const DeferredDisposal&) = delete;
        DeferredDisposal& operator=(const DeferredDisposal&) = delete;
        DeferredDisposal(DeferredDisposal&&) = delete;
        DeferredDisposal& operator=(DeferredDisposal&&) = delete;

        DisposalPayload payload{};

    private:
        friend class SEDevice;
        std::unique_ptr<DeferredDisposal> next{};
    };
}
