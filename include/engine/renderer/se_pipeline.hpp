#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <cassert>

namespace se
{
    class SEDevice;

    struct PipelineConfigInfo
    {
        PipelineConfigInfo() = default;
        PipelineConfigInfo(const PipelineConfigInfo &) = delete;
        PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

        VkPipelineViewportStateCreateInfo viewportInfo{};
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
        VkRenderPass renderPass{VK_NULL_HANDLE};
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
        SEPipeline(SEPipeline&& other) noexcept;
        SEPipeline& operator=(SEPipeline&&) = delete;

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
        void destroyOwnedHandles() noexcept;

        SEDevice &seDevice;
        VkPipeline     graphicsPipeline = VK_NULL_HANDLE;
        VkPipeline     computePipeline  = VK_NULL_HANDLE;
        VkShaderModule vertShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragShaderModule = VK_NULL_HANDLE;
        VkShaderModule compShaderModule = VK_NULL_HANDLE;
    };
}
