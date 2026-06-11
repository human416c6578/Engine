#pragma once

#include "engine/core/se_device.hpp"
#include "engine/core/se_vertex.hpp"

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <cassert>

namespace se
{

    struct PipelineConfigInfo
    {
        PipelineConfigInfo() = default;
        PipelineConfigInfo(const PipelineConfigInfo &) = delete;
        PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;

        std::vector<VkVertexInputBindingDescription>   bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    };

    class SEPipeline
    {
    public:
        SEPipeline(
            SEDevice &device,
            const std::string &vertFilepath,
            const std::string &fragFilepath,
            const PipelineConfigInfo &configInfo,
            const VkSampleCountFlagBits msaaSamples);

        SEPipeline(
            SEDevice& device,
            const std::string& compFilepath,
            VkPipelineLayout computePipelineLayout);

		~SEPipeline();

		SEPipeline(const SEPipeline &) = delete;
        SEPipeline &operator=(const SEPipeline &) = delete;

        void bind(VkCommandBuffer commandBuffer);
        void bindCompute(VkCommandBuffer commandBuffer);
		VkPipeline getPipeline() const { return graphicsPipeline; }

        static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo);
        static void particlePipelineConfigInfo(PipelineConfigInfo &configInfo);


    private:
        static std::vector<char> readFile(const std::string &filepath);

        void createGraphicsPipeline(
            const std::string &vertFilepath,
            const std::string &fragFilepath,
            const PipelineConfigInfo &configInfo);

        void createComputePipeline(
            const std::string& compFilepath,
            VkPipelineLayout layout);

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

        SEDevice &seDevice;
        VkPipeline graphicsPipeline;
        VkPipeline computePipeline;  
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        VkShaderModule compShaderModule;
    };
}