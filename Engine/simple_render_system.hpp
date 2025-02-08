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
    class SimpleRenderSystem
    {
    public:
        SimpleRenderSystem(SEDevice &device, VkRenderPass renderPass);
        ~SimpleRenderSystem();

        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

        void renderGameObjects(
            VkCommandBuffer commandBuffer,
            std::vector<SEGameObject> &gameObjects,
            const SECamera &camera,
            se::SEGameObject &viewerObject);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);
        void createDescriptorSets();
        void createDescriptorSetLayout();

        void bind(VkCommandBuffer commandBuffer)
        {
            sePipeline->bind(commandBuffer);

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,
                1,
                &descriptorSet,
                0,
                nullptr);
        }

        SEDevice &seDevice;
        VkDescriptorSet descriptorSet;
        VkDescriptorSetLayout descriptorSetLayout;

        std::unique_ptr<SEPipeline> sePipeline;
        VkPipelineLayout pipelineLayout;
    };
}