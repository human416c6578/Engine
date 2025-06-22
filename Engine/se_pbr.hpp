#pragma once

#include "se_camera.hpp"
#include "se_device.hpp"
#include "se_gameobject.hpp"
#include "se_cubemap.hpp"
#include "se_pipeline.hpp"

// std
#include <memory>
#include <vector>

namespace se
{
	constexpr int MAX_LIGHTS = 100;

    struct alignas(16) LightUBO {
        alignas(4) int lightCount;
        alignas(16) glm::vec3 padding;
        alignas(16) Light lights[MAX_LIGHTS];
    };

    struct Buffer {
        VkBuffer buffer;
        VkDeviceMemory memory;
        void* mapped;
    };

    class PBR
    {
    public:
        PBR(SEDevice& device, VkRenderPass renderPass, SECubemap& cubemap);

        ~PBR();

        PBR(const PBR&) = delete;
        PBR& operator=(const PBR&) = delete;

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

        void updateLightsBuffer(int frameIndex, std::vector<Light> lights);

        void createGlobalDescriptorSetLayout();
        void createMaterialDescriptorSetLayout();

        void createUniformBuffer();

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
        VkDescriptorSetLayout globalDescriptorSetLayout;
        VkDescriptorSetLayout materialDescriptorSetLayout;

        std::unique_ptr<SEPipeline> sePipeline;
        VkPipelineLayout pipelineLayout;

		std::vector<Buffer> lightBuffers;
        std::vector<bool> needUpdate;
		
    };


}
