#pragma once

#include "engine/core/se_camera.hpp"
#include "engine/vulkan/se_device.hpp"
#include "engine/scene/se_gameobject.hpp"
#include "engine/renderer/se_cubemap.hpp"
#include "engine/renderer/se_pipeline.hpp"

// std
#include <memory>
#include <vector>
#include <list>
#include <utility>

namespace se
{
	constexpr int MAX_LIGHTS = 100;

    struct alignas(16) LightUBO {
        alignas(4) int lightCount;
        alignas(16) glm::vec3 padding;
        alignas(16) Light lights[MAX_LIGHTS];
    };

    struct Buffer {
        explicit Buffer(SEDevice& owner) noexcept : device{&owner} {}
        ~Buffer() noexcept { reset(); }

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&& other) noexcept
            : device{std::exchange(other.device, nullptr)},
              buffer{std::exchange(other.buffer, VK_NULL_HANDLE)},
              memory{std::exchange(other.memory, VK_NULL_HANDLE)},
              mapped{std::exchange(other.mapped, nullptr)}
        {
        }
        Buffer& operator=(Buffer&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                device = std::exchange(other.device, nullptr);
                buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
                memory = std::exchange(other.memory, VK_NULL_HANDLE);
                mapped = std::exchange(other.mapped, nullptr);
            }
            return *this;
        }

        void reset() noexcept
        {
            if (device != nullptr && device->device() != VK_NULL_HANDLE)
            {
                if (mapped != nullptr && memory != VK_NULL_HANDLE)
                    vkUnmapMemory(device->device(), memory);
                if (buffer != VK_NULL_HANDLE)
                    vkDestroyBuffer(device->device(), buffer, nullptr);
                if (memory != VK_NULL_HANDLE)
                    vkFreeMemory(device->device(), memory, nullptr);
            }
            buffer = VK_NULL_HANDLE;
            memory = VK_NULL_HANDLE;
            mapped = nullptr;
        }

        SEDevice* device{nullptr};
        VkBuffer buffer{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
        void* mapped{nullptr};
    };

    class PBR
    {
    public:
        PBR(SEDevice& device, VkRenderPass renderPass, SECubemap& cubemap);

        ~PBR();

        PBR(const PBR&) = delete;
        PBR& operator=(const PBR&) = delete;
        PBR(PBR&&) = delete;
        PBR& operator=(PBR&&) = delete;

        VkDescriptorSetLayout getMaterialDescriptorSetLayout() { return materialDescriptorSetLayout; }
		VkPipelineLayout getPipelineLayout() { return pipelineLayout; }
        VkPipeline getPipeline() { return sePipeline->getPipeline(); }

        void renderGameObjects(VkCommandBuffer commandBuffer, const std::list<std::unique_ptr<se::SEGameObject>>& gameObjects, int frameIndex);
        void renderCubeMap(VkCommandBuffer commandBuffer);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);
        void createDescriptorSets();

        void updateDescriptorSet(size_t frameIndex);

        void updateLightsBuffer(int frameIndex, const std::vector<Light>& lights);

        void createGlobalDescriptorSetLayout();
        void createMaterialDescriptorSetLayout();

        void createUniformBuffer();
        void destroyOwnedHandles() noexcept;

        void bind(VkCommandBuffer commandBuffer, int frameIndex)
        {
            sePipeline->bind(commandBuffer);

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,  // Set 0 (Global UBO)
                1,
                &descriptorSets[frameIndex],
                0,
                nullptr);
        }

        SEDevice& seDevice;
        SECubemap& seCubemap;
        std::vector<VkDescriptorSet> descriptorSets;
        VkDescriptorSetLayout globalDescriptorSetLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout materialDescriptorSetLayout{VK_NULL_HANDLE};

        std::unique_ptr<SEPipeline> sePipeline;
        VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

		std::vector<Buffer> lightBuffers;
        std::vector<bool> needUpdate;
		
    };


}
