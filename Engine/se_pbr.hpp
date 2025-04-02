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
    class PBR
    {
    public:
        PBR(SEDevice& device, VkRenderPass renderPass, SECubemap& cubemap);

        ~PBR();

        PBR(const PBR&) = delete;
        PBR& operator=(const PBR&) = delete;

        VkDescriptorSetLayout getMaterialDescriptorSetLayout() { return materialDescriptorSetLayout; }

        void renderGameObjects(
            VkCommandBuffer commandBuffer,
            std::vector<SEGameObject>& gameObjects,
            const SECamera& camera,
            se::SEGameObject& viewerObject);
        
        void renderCubeMap(VkCommandBuffer commandBuffer);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);
        void createDescriptorSets();
        void createGlobalDescriptorSetLayout();
        void createMaterialDescriptorSetLayout();

        void bind(VkCommandBuffer commandBuffer)
        {
            sePipeline->bind(commandBuffer);

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,  // Set 0 (Global UBO)
                1,
                &descriptorSet,
                0,
                nullptr);
        }

        SEDevice& seDevice;
        SECubemap& seCubemap;
        VkDescriptorSet descriptorSet;
        VkDescriptorSetLayout globalDescriptorSetLayout;
        VkDescriptorSetLayout materialDescriptorSetLayout;

        std::unique_ptr<SEPipeline> sePipeline;
        VkPipelineLayout pipelineLayout;
    };


}
