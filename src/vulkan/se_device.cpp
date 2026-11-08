#include "engine/vulkan/se_device.hpp"
#include "engine/core/se_logger.hpp"
#include "engine/vulkan/se_vk_result.hpp"
#include "engine/platform/se_window.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace se
{
    namespace
    {
        constexpr std::array<const char*, 1> kValidationLayers{"VK_LAYER_KHRONOS_validation"};
        constexpr std::array<const char*, 1> kDeviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL SEDevice::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData)
    {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            log::error("Validation", "%s", callbackData->pMessage);
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            log::warn("Validation", "%s", callbackData->pMessage);
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            log::info("Validation", "%s", callbackData->pMessage);
        else
            log::debug("Validation", "%s", callbackData->pMessage);
        return VK_FALSE;
    }
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    SEDevice::SEDevice(SEWindow &win) : window{win}
    {
        try
        {
            createInstance();
            setupDebugMessenger();
            createSurface(instance, &surface_);
            pickPhysicalDevice();
            createLogicalDevice();
            createCommandPool();
            createDescriptorPool();
            createUniformBuffers();

            VkDescriptorSetLayoutBinding binding{};
            binding.binding = 0;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &binding;

            checkVkResult(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &imGuiDescriptorSetLayout),
                "vkCreateDescriptorSetLayout(ImGui)");
        }
        catch (...)
        {
            destroyOwnedHandles();
            throw;
        }
    }

    void SEDevice::logResult(VkResult result, std::string_view operation) const noexcept
    {
        if (result != VK_SUCCESS)
            log::error("Vulkan", "%.*s failed with VkResult %d",
                static_cast<int>(operation.size()), operation.data(), static_cast<int>(result));
    }

    SEDevice::~SEDevice()
    {
        destroyOwnedHandles();
    }

    void SEDevice::destroyOwnedHandles() noexcept
    {
        if (device_ != VK_NULL_HANDLE)
        {
            const VkResult idleResult = vkDeviceWaitIdle(device_);
            logResult(idleResult, "vkDeviceWaitIdle(SEDevice teardown)");

            if (descriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(device_, descriptorPool, nullptr);
                descriptorPool = VK_NULL_HANDLE;
            }

            if (imGuiDescriptorSetLayout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(device_, imGuiDescriptorSetLayout, nullptr);
                imGuiDescriptorSetLayout = VK_NULL_HANDLE;
            }

            for (size_t i = 0; i < uniformBuffers.size(); ++i)
            {
                if (i < uniformBuffersMapped.size() && uniformBuffersMapped[i] != nullptr &&
                    i < uniformBuffersMemory.size() && uniformBuffersMemory[i] != VK_NULL_HANDLE)
                {
                    vkUnmapMemory(device_, uniformBuffersMemory[i]);
                    uniformBuffersMapped[i] = nullptr;
                }
                if (uniformBuffers[i] != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(device_, uniformBuffers[i], nullptr);
                    uniformBuffers[i] = VK_NULL_HANDLE;
                }
                if (i < uniformBuffersMemory.size() && uniformBuffersMemory[i] != VK_NULL_HANDLE)
                {
                    vkFreeMemory(device_, uniformBuffersMemory[i], nullptr);
                    uniformBuffersMemory[i] = VK_NULL_HANDLE;
                }
            }

            if (commandPool != VK_NULL_HANDLE)
            {
                vkDestroyCommandPool(device_, commandPool, nullptr);
                commandPool = VK_NULL_HANDLE;
            }

            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }

        if (instance != VK_NULL_HANDLE)
        {
            if (surface_ != VK_NULL_HANDLE)
            {
                vkDestroySurfaceKHR(instance, surface_, nullptr);
                surface_ = VK_NULL_HANDLE;
            }
            if (debugMessenger != VK_NULL_HANDLE)
            {
                DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
                debugMessenger = VK_NULL_HANDLE;
            }
            vkDestroyInstance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }

        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();
    }

    void SEDevice::createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Shark Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Shark Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
            createInfo.ppEnabledLayerNames = kValidationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        checkVkResult(vkCreateInstance(&createInfo, nullptr, &instance), "vkCreateInstance");

        if (enableValidationLayers)
        {
            hasGflwRequiredInstanceExtensions();
        }
    }

    void SEDevice::createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        poolInfo.flags =
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        checkVkResult(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool), "vkCreateCommandPool");
    }

    void SEDevice::createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 5000;
        poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 5000;
        poolSizes[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = 5000;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = 5000;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        checkVkResult(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool), "vkCreateDescriptorPool");
    }

    void SEDevice::createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

            checkVkResult(vkMapMemory(device_, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]),
                "vkMapMemory(uniform buffer)");
        }
    }

    void SEDevice::updateUniformBuffers(std::size_t frameIndex, const UniformBufferObject& bufferObject)
    {
        if (frameIndex >= uniformBuffersMapped.size())
        {
            throw std::out_of_range("uniform buffer frame index is out of range");
        }
        memcpy(uniformBuffersMapped[frameIndex], &bufferObject, sizeof(bufferObject));
    }

    void SEDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        // Describe the buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Create the buffer
        buffer = VK_NULL_HANDLE;
        bufferMemory = VK_NULL_HANDLE;
        checkVkResult(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer), "vkCreateBuffer");

        // Get memory requirements for the buffer
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

        // Set up memory allocation info
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        // Allocate memory for the buffer
        try
        {
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
            checkVkResult(vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory), "vkAllocateMemory(buffer)");
            checkVkResult(vkBindBufferMemory(device_, buffer, bufferMemory, 0), "vkBindBufferMemory");
        }
        catch (...)
        {
            if (bufferMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(device_, bufferMemory, nullptr);
                bufferMemory = VK_NULL_HANDLE;
            }
            if (buffer != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(device_, buffer, nullptr);
                buffer = VK_NULL_HANDLE;
            }
            throw;
        }
    }

    void SEDevice::createSurface(VkInstance instance, VkSurfaceKHR *surface)
    {
        window.createWindowSurface(instance, surface);
    }

    void SEDevice::pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        checkVkResult(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr), "vkEnumeratePhysicalDevices(count)");

        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        checkVkResult(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()), "vkEnumeratePhysicalDevices(data)");

        for (const auto &device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                msaaSamples = getMaxUsableSampleCount();
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void SEDevice::createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

        checkVkResult(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_), "vkCreateDevice");

        vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
        vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
    }

    SwapChainSupportDetails SEDevice::querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        checkVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities),
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

        uint32_t formatCount = 0;
        checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr),
            "vkGetPhysicalDeviceSurfaceFormatsKHR(count)");

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data()),
                "vkGetPhysicalDeviceSurfaceFormatsKHR(data)");
        }

        uint32_t presentModeCount = 0;
        checkVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr),
            "vkGetPhysicalDeviceSurfacePresentModesKHR(count)");

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            checkVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data()),
                "vkGetPhysicalDeviceSurfacePresentModesKHR(data)");
        }

        return details;
    }

    bool SEDevice::isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    bool SEDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount = 0;
        checkVkResult(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr),
            "vkEnumerateDeviceExtensionProperties(count)");

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        checkVkResult(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()),
            "vkEnumerateDeviceExtensionProperties(data)");

        std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());

        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices SEDevice::findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            checkVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport),
                "vkGetPhysicalDeviceSurfaceSupportKHR");

            if (presentSupport)
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    void SEDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    void SEDevice::setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        checkVkResult(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger),
            "vkCreateDebugUtilsMessengerEXT");
    }

    bool SEDevice::checkValidationLayerSupport()
    {
        uint32_t layerCount = 0;
        checkVkResult(vkEnumerateInstanceLayerProperties(&layerCount, nullptr),
            "vkEnumerateInstanceLayerProperties(count)");

        std::vector<VkLayerProperties> availableLayers(layerCount);
        checkVkResult(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()),
            "vkEnumerateInstanceLayerProperties(data)");

        for (const char *layerName : kValidationLayers)
        {
            bool layerFound = false;

            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }
            
            if (!layerFound)
            {
                throw std::runtime_error("failed to find layer " + std::string(layerName));
                return false;
            }
        }

        return true;
    }

    std::vector<const char *> SEDevice::getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    VkSampleCountFlagBits SEDevice::getMaxUsableSampleCount()
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT)
        {
            return VK_SAMPLE_COUNT_64_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_32_BIT)
        {
            return VK_SAMPLE_COUNT_32_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_16_BIT)
        {
            return VK_SAMPLE_COUNT_16_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_8_BIT)
        {
            return VK_SAMPLE_COUNT_8_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_4_BIT)
        {
            return VK_SAMPLE_COUNT_4_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_2_BIT)
        {
            return VK_SAMPLE_COUNT_2_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VkCommandBuffer SEDevice::beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        checkVkResult(vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer), "vkAllocateCommandBuffers");

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        checkVkResult(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");

        return commandBuffer;
    }

    void SEDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        checkVkResult(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        checkVkResult(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE), "vkQueueSubmit(single-time)");
        checkVkResult(vkQueueWaitIdle(graphicsQueue_), "vkQueueWaitIdle(single-time)");

        vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
    }

    void SEDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void SEDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = layerCount;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    void SEDevice::createImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
    {
        image = VK_NULL_HANDLE;
        imageMemory = VK_NULL_HANDLE;
        checkVkResult(vkCreateImage(device_, &imageInfo, nullptr, &image), "vkCreateImage");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device_, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        try
        {
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
            checkVkResult(vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory), "vkAllocateMemory(image)");
            checkVkResult(vkBindImageMemory(device_, image, imageMemory, 0), "vkBindImageMemory");
        }
        catch (...)
        {
            if (imageMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(device_, imageMemory, nullptr);
                imageMemory = VK_NULL_HANDLE;
            }
            if (image != VK_NULL_HANDLE)
            {
                vkDestroyImage(device_, image, nullptr);
                image = VK_NULL_HANDLE;
            }
            throw;
        }
    }

    uint32_t SEDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkFormat SEDevice::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat SEDevice::findDepthFormat()
    {
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void SEDevice::hasGflwRequiredInstanceExtensions()
    {
        uint32_t extensionCount = 0;
        checkVkResult(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr),
            "vkEnumerateInstanceExtensionProperties(count)");
        std::vector<VkExtensionProperties> extensions(extensionCount);
        checkVkResult(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()),
            "vkEnumerateInstanceExtensionProperties(data)");

        log::debug("Vulkan", "available extensions:");
        std::unordered_set<std::string> available;
        for (const auto &extension : extensions)
        {
            log::debug("Vulkan", "%s", extension.extensionName);
            available.insert(extension.extensionName);
        }

        log::debug("Vulkan", "required extensions:");
        auto requiredExtensions = getRequiredExtensions();
        for (const auto &required : requiredExtensions)
        {
            log::debug("Vulkan", "%s", required);
            if (available.find(required) == available.end())
            {
                throw std::runtime_error("Missing required glfw extension");
            }
        }
    }

    uint32_t SEDevice::findMemoryType(VkDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

}
