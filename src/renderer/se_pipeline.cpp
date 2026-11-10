#include "engine/renderer/se_pipeline.hpp"
#include "engine/core/se_logger.hpp"
#include "engine/renderer/se_vertex.hpp"
#include "engine/vulkan/se_device.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include "engine/vulkan/se_vk_result.hpp"

#include <utility>

namespace se
{
    SEPipeline::SEPipeline(
        SEDevice &device,
        const std::string &vertFilepath,
        const std::string &fragFilepath,
        const PipelineConfigInfo &configInfo,
        const VkSampleCountFlagBits msaaSamples
        )
        : seDevice{device}
    {
        try
        {
            createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
        }
        catch (...)
        {
            destroyOwnedHandles();
            throw;
        }
    }

    SEPipeline::SEPipeline(
        SEDevice& device,
        const std::string& compFilepath,
        VkPipelineLayout computePipelineLayout)
        : seDevice{ device }
    {
        try
        {
            createComputePipeline(compFilepath, computePipelineLayout);
        }
        catch (...)
        {
            destroyOwnedHandles();
            throw;
        }
    }

    SEPipeline::SEPipeline(SEPipeline&& other) noexcept
        : seDevice{other.seDevice},
          graphicsPipeline{std::exchange(other.graphicsPipeline, VK_NULL_HANDLE)},
          computePipeline{std::exchange(other.computePipeline, VK_NULL_HANDLE)},
          vertShaderModule{std::exchange(other.vertShaderModule, VK_NULL_HANDLE)},
          fragShaderModule{std::exchange(other.fragShaderModule, VK_NULL_HANDLE)},
          compShaderModule{std::exchange(other.compShaderModule, VK_NULL_HANDLE)}
    {
    }

    SEPipeline::~SEPipeline()
    {
        destroyOwnedHandles();
    }

    void SEPipeline::destroyOwnedHandles() noexcept
    {
        if (graphicsPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(seDevice.device(), graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
        if (computePipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(seDevice.device(), computePipeline, nullptr);
        computePipeline = VK_NULL_HANDLE;
        if (vertShaderModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(seDevice.device(), vertShaderModule, nullptr);
        vertShaderModule = VK_NULL_HANDLE;
        if (fragShaderModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(seDevice.device(), fragShaderModule, nullptr);
        fragShaderModule = VK_NULL_HANDLE;
        if (compShaderModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(seDevice.device(), compShaderModule, nullptr);
        compShaderModule = VK_NULL_HANDLE;
    }

    void SEPipeline::bind(VkCommandBuffer commandBuffer)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }

    void SEPipeline::bindCompute(VkCommandBuffer commandBuffer)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    }

    void SEPipeline::defaultPipelineConfigInfo(PipelineConfigInfo &configInfo)
    {
        configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        configInfo.viewportInfo.viewportCount = 1;
        configInfo.viewportInfo.pViewports = nullptr;
        configInfo.viewportInfo.scissorCount = 1;
        configInfo.viewportInfo.pScissors = nullptr;

        configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
        configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        configInfo.rasterizationInfo.lineWidth = 1.0f;
        configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        //configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
        configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
        configInfo.rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
        configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

        configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
        configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        configInfo.multisampleInfo.minSampleShading = 1.0f;          // Optional
        configInfo.multisampleInfo.pSampleMask = nullptr;            // Optional
        configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
        configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional

        configInfo.colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
        configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
        configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

        configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
        configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
        configInfo.colorBlendInfo.attachmentCount = 1;
        configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
        configInfo.colorBlendInfo.blendConstants[0] = 0.0f; // Optional
        configInfo.colorBlendInfo.blendConstants[1] = 0.0f; // Optional
        configInfo.colorBlendInfo.blendConstants[2] = 0.0f; // Optional
        configInfo.colorBlendInfo.blendConstants[3] = 0.0f; // Optional

        configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
        configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
        configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.minDepthBounds = 0.0f; // Optional
        configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // Optional
        configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.front = {}; // Optional
        configInfo.depthStencilInfo.back = {};  // Optional
        
        configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
        configInfo.dynamicStateInfo.dynamicStateCount =
            static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
        configInfo.dynamicStateInfo.flags = 0;

        configInfo.bindingDescriptions   = Vertex::getBindingDescriptions();
        configInfo.attributeDescriptions = Vertex::getAttributeDescriptions();
    }

    void SEPipeline::particlePipelineConfigInfo(PipelineConfigInfo& configInfo)
    {
        defaultPipelineConfigInfo(configInfo);

        configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        configInfo.bindingDescriptions.clear();
        configInfo.attributeDescriptions.clear();

        configInfo.colorBlendAttachment.blendEnable         = VK_TRUE;
        configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;         // additive
        configInfo.colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
        configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        configInfo.colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

        configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
        configInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
        configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.minDepthBounds = 0.0f; // Optional
        configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // Optional
        configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.front = {}; // Optional
        configInfo.depthStencilInfo.back = {};  // Optional
    }

    static std::vector<char> readFile(const std::string &filepath)
    {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);
        
        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    std::vector<char> SEPipeline::readFile(const std::string &filepath)
    {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions()
    {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        return attributeDescriptions;
    }

    void SEPipeline::createGraphicsPipeline(
        const std::string &vertFilepath,
        const std::string &fragFilepath,
        const PipelineConfigInfo &configInfo)
    {
        log::debug("Pipeline", "creating: %s | bindings: %zu | attribs: %zu | topology: %d",
            vertFilepath.c_str(),
            configInfo.bindingDescriptions.size(),
            configInfo.attributeDescriptions.size(),
            configInfo.inputAssemblyInfo.topology);
        assert(
            configInfo.pipelineLayout != VK_NULL_HANDLE &&
            "Cannot create graphics pipeline: no pipelineLayout provided in configInfo");
        assert(
            configInfo.renderPass != VK_NULL_HANDLE &&
            "Cannot create graphics pipeline: no renderPass provided in configInfo");

        auto vertCode = readFile(vertFilepath);
        auto fragCode = readFile(fragFilepath);

        createShaderModule(vertCode, &vertShaderModule);
        createShaderModule(fragCode, &fragShaderModule);

        VkPipelineShaderStageCreateInfo shaderStages[2];
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "main";
        shaderStages[0].flags = 0;
        shaderStages[0].pNext = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "main";
        shaderStages[1].flags = 0;
        shaderStages[1].pNext = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount   = static_cast<uint32_t>(configInfo.bindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions      = configInfo.bindingDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(configInfo.attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions    = configInfo.attributeDescriptions.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
        pipelineInfo.pViewportState = &configInfo.viewportInfo;
        pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
        pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
        pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
        pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
        pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

        pipelineInfo.layout = configInfo.pipelineLayout;
        pipelineInfo.renderPass = configInfo.renderPass;
        pipelineInfo.subpass = configInfo.subpass;

        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        checkVkResult(vkCreateGraphicsPipelines(
                seDevice.device(),
                VK_NULL_HANDLE,
                1,
                &pipelineInfo,
                nullptr,
                &graphicsPipeline), "vkCreateGraphicsPipelines");
    }

    void SEPipeline::createComputePipeline(
        const std::string& compFilepath,
        VkPipelineLayout layout)
    {
        assert(layout != VK_NULL_HANDLE &&
            "Cannot create compute pipeline: no pipelineLayout provided");

        auto compCode = readFile(compFilepath);
        createShaderModule(compCode, &compShaderModule);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = compShaderModule;
        stageInfo.pName  = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage  = stageInfo;
        pipelineInfo.layout = layout;

        checkVkResult(vkCreateComputePipelines(
                seDevice.device(),
                VK_NULL_HANDLE,
                1, &pipelineInfo,
                nullptr,
                &computePipeline), "vkCreateComputePipelines");
    }

    void SEPipeline::createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        checkVkResult(vkCreateShaderModule(seDevice.device(), &createInfo, nullptr, shaderModule), "vkCreateShaderModule");
    }
}