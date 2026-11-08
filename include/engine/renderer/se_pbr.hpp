#pragma once

#include "engine/core/se_camera.hpp" // Light is embedded by value in LightUBO.
#include "engine/renderer/se_pipeline.hpp" // PBR invokes SEPipeline in inline methods.
#include <vulkan/vulkan.h> // Vulkan handle types appear in this interface.

// std
#include <memory>
#include <vector>
#include <list>

namespace se
{
    class SEDevice;
    class SEGameObject;
    class SECubemap;

	constexpr int MAX_LIGHTS = 100;

    struct alignas(16) LightUBO {
        alignas(4) int lightCount;
        alignas(16) glm::vec3 padding;
        alignas(16) Light lights[MAX_LIGHTS];
    };

    struct Buffer {
        explicit Buffer(SEDevice& owner) noexcept;
        ~Buffer() noexcept;

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        void reset() noexcept;

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
