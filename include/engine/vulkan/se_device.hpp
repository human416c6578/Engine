#pragma once

#include <string_view>
#include "engine/core/se_frame_config.hpp" // SEDevice sizes its frame resources using MAX_FRAMES_IN_FLIGHT.

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#ifndef ENGINE_ENABLE_VALIDATION
#define ENGINE_ENABLE_VALIDATION 0
#endif

inline constexpr bool enableValidationLayers = ENGINE_ENABLE_VALIDATION != 0;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct UniformBufferObject
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPos;
};

namespace se
{
    class SEWindow;
    struct DeferredDisposal;

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class SEDevice
    {
    public:
        SEDevice(SEWindow &win);
        ~SEDevice();

        // Not copyable or movable
        SEDevice(const SEDevice &) = delete;
        SEDevice &operator=(const SEDevice &) = delete;
        SEDevice(SEDevice &&) = delete;
        SEDevice &operator=(SEDevice &&) = delete;

        void logResult(VkResult result, std::string_view operation) const noexcept;

        // Main-thread only. Reserve before acquiring handles; retire never allocates.
        std::unique_ptr<DeferredDisposal> makeDisposalRecord();
        void retire(std::unique_ptr<DeferredDisposal> record) noexcept;

        // Scheduler supplies slots; SESwapChain alone owns/advances fences and indices.
        void openDisposals(uint32_t firstSlot) noexcept;
        void beginDisposalFrameAfterFence(uint32_t slot) noexcept;
        void markDisposalFrameSubmitted(uint32_t slot) noexcept;
        // Call only after permanently discarding the unsubmitted recording.
        void abandonDisposalRecording() noexcept;
        // Successful device idle AND no recording that can later be submitted required.
        void drainAllDisposalsAfterIdle() noexcept;
        void closeDisposalsAfterIdle() noexcept;

        VkCommandPool getCommandPool() { return commandPool; }
        VkDescriptorPool getDescriptorPool() { return descriptorPool; }
        std::vector<VkBuffer> getUniformBuffers() { return uniformBuffers; }
        VkDevice device() { return device_; }
        VkPhysicalDevice physicaldevice() { return physicalDevice; }
        VkSurfaceKHR surface() { return surface_; }
        VkInstance getInstance() { return instance; }
        VkQueue graphicsQueue() { return graphicsQueue_; }
        VkQueue presentQueue() { return presentQueue_; }

		VkDescriptorSetLayout getImGuiDescriptorSetLayout() { return imGuiDescriptorSetLayout; }

        void updateUniformBuffers(std::size_t frameIndex, const UniformBufferObject& bufferObject);

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
        VkFormat findSupportedFormat(
            const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();
        // Buffer Helper Functions
        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer &buffer,
            VkDeviceMemory &bufferMemory);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void copyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

        void createImageWithInfo(
            const VkImageCreateInfo &imageInfo,
            VkMemoryPropertyFlags properties,
            VkImage &image,
            VkDeviceMemory &imageMemory);

        VkPhysicalDeviceProperties properties{};

    private:
        enum class DisposalState { Closed, AwaitingFirstSubmission, Recording, Submitted, Abandoned };
        struct DisposalList
        {
            DisposalList() noexcept;
            ~DisposalList() noexcept;

            std::unique_ptr<DeferredDisposal> head{};
            DeferredDisposal* tail{nullptr}; // Borrow into head's chain.
            std::array<uint64_t, 3> enqueued{};
            std::array<uint64_t, 3> disposed{};
        };
        const std::thread::id disposalThread{std::this_thread::get_id()};
        std::array<DisposalList, MAX_FRAMES_IN_FLIGHT> disposalLists{};
        DisposalState disposalState{DisposalState::Closed};
        uint32_t disposalSlot{0}; // Copied scheduler context, never independently advanced.
        uint64_t immediateDisposals{0};

        void requireDisposal(bool condition, const char* operation) const noexcept;
        void drainDisposalLists(std::size_t first, std::size_t end, const char* completion) noexcept;
        void disposeRecordPass(DeferredDisposal& record, unsigned pass) noexcept;

        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VkDevice device_ = VK_NULL_HANDLE;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;
        VkQueue graphicsQueue_ = VK_NULL_HANDLE;
        VkQueue presentQueue_ = VK_NULL_HANDLE;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;

		VkDescriptorSetLayout imGuiDescriptorSetLayout = VK_NULL_HANDLE;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void *> uniformBuffersMapped;


        VkCommandPool commandPool = VK_NULL_HANDLE;

        SEWindow &window;

        void createInstance();
        void setupDebugMessenger();
        void createSurface(VkInstance instance, VkSurfaceKHR *surface);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();
        void createDescriptorPool();
		void destroyOwnedHandles() noexcept;

		void createUniformBuffers();

		bool isDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char *> getRequiredExtensions();
        bool checkValidationLayerSupport();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        void hasGflwRequiredInstanceExtensions();
		uint32_t findMemoryType(VkDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkSampleCountFlagBits getMaxUsableSampleCount();
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
            void* userData);
    };
}
