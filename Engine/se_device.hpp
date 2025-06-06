#pragma once

#include "se_window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string.h>
#include <vector>
#include <optional>
#include <stdexcept>
#include <set>
#include <unordered_set>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

//#define NDEBUG

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
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
        
        VkCommandPool getCommandPool() { return commandPool; }
        VkDescriptorPool getDescriptorPool() { return descriptorPool; }
        std::vector<VkBuffer> getUniformBuffers() { return uniformBuffers; }
        VkDevice device() { return device_; }
        VkPhysicalDevice physicaldevice() { return physicalDevice; }
        VkSurfaceKHR surface() { return surface_; }
        VkInstance getInstance() { return instance; }
        VkQueue graphicsQueue() { return graphicsQueue_; }
        VkQueue presentQueue() { return presentQueue_; }

        void updateUniformBuffers(UniformBufferObject bufferObject);

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

        VkPhysicalDeviceProperties properties;

    private:
        const int MAX_FRAMES_IN_FLIGHT = 2;

        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        VkQueue presentQueue_; 

        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void *> uniformBuffersMapped;
        

        VkCommandPool commandPool;

        SEWindow &window;

        void createInstance();
        void setupDebugMessenger();
        void createSurface(VkInstance instance, VkSurfaceKHR *surface);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();
        void createDescriptorPool();

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

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
        {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
        }
    };
}
