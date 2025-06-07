#include "se_pbr.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace se
{
    PBR::PBR(SEDevice& device, VkRenderPass renderPass, SECubemap& cubemap)
        : seDevice{ device }, seCubemap{ cubemap }
    {

        createGlobalDescriptorSetLayout();
        createMaterialDescriptorSetLayout();
        createDescriptorSets();
        createPipelineLayout();
        createPipeline(renderPass);
    }

    PBR::~PBR()
    {
        vkDestroyDescriptorSetLayout(seDevice.device(), globalDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(seDevice.device(), materialDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(seDevice.device(), pipelineLayout, nullptr);
    }

    void PBR::createPipelineLayout()
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
        globalDescriptorSetLayout,   // Set 0: Global (UBO)
        materialDescriptorSetLayout  // Set 1: Material (Textures, Properties)
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(seDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void PBR::createPipeline(VkRenderPass renderPass)
    {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        SEPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        sePipeline = std::make_unique<SEPipeline>(
            seDevice,
            "shaders/pbrVert.spv",
            "shaders/pbrFrag.spv",
            pipelineConfig,
            VK_SAMPLE_COUNT_1_BIT);
    }

    void PBR::createGlobalDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        VkDescriptorSetLayoutBinding diffuseLayoutBinding{};
        diffuseLayoutBinding.binding = 0;
        diffuseLayoutBinding.descriptorCount = 1;
        diffuseLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        diffuseLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.push_back(diffuseLayoutBinding);

        VkDescriptorSetLayoutBinding specularLayoutBinding{};
        specularLayoutBinding.binding = 1;
        specularLayoutBinding.descriptorCount = 1;
        specularLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        specularLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.push_back(specularLayoutBinding);

        VkDescriptorSetLayoutBinding BRDFLayoutBinding{};
        BRDFLayoutBinding.binding = 2;
        BRDFLayoutBinding.descriptorCount = 1;
        BRDFLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        BRDFLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.push_back(BRDFLayoutBinding);

        // Descriptor set layout create info
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        // Create the descriptor set layout
        if (vkCreateDescriptorSetLayout(seDevice.device(), &layoutInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void PBR::createMaterialDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        // UBO layout binding (always present)
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(uboLayoutBinding);

        // Material properties layout binding (always present)
        VkDescriptorSetLayoutBinding matLayoutBinding{};
        matLayoutBinding.binding = 1;
        matLayoutBinding.descriptorCount = 1;
        matLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        matLayoutBinding.pImmutableSamplers = nullptr;
        matLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(matLayoutBinding);

        // Diffuse (Albedo) texture binding (always present)
        VkDescriptorSetLayoutBinding albedoLayoutBinding{};
        albedoLayoutBinding.binding = 2;
        albedoLayoutBinding.descriptorCount = 1;
        albedoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        albedoLayoutBinding.pImmutableSamplers = nullptr;
        albedoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(albedoLayoutBinding);

        VkDescriptorSetLayoutBinding normalLayoutBinding{};
        normalLayoutBinding.binding = 3;
        normalLayoutBinding.descriptorCount = 1;
        normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normalLayoutBinding.pImmutableSamplers = nullptr;
        normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(normalLayoutBinding);

        VkDescriptorSetLayoutBinding metallicLayoutBinding{};
        metallicLayoutBinding.binding = 4;
        metallicLayoutBinding.descriptorCount = 1;
        metallicLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        metallicLayoutBinding.pImmutableSamplers = nullptr;
        metallicLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(metallicLayoutBinding);

        VkDescriptorSetLayoutBinding roughnessLayoutBinding{};
        roughnessLayoutBinding.binding = 5;
        roughnessLayoutBinding.descriptorCount = 1;
        roughnessLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        roughnessLayoutBinding.pImmutableSamplers = nullptr;
        roughnessLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(roughnessLayoutBinding);

        VkDescriptorSetLayoutBinding aoLayoutBinding{};
        aoLayoutBinding.binding = 6;
        aoLayoutBinding.descriptorCount = 1;
        aoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        aoLayoutBinding.pImmutableSamplers = nullptr;
        aoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(aoLayoutBinding);

        // Descriptor set layout create info
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        // Create the descriptor set layout
        if (vkCreateDescriptorSetLayout(seDevice.device(), &layoutInfo, nullptr, &materialDescriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void PBR::createDescriptorSets()
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = seDevice.getDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &globalDescriptorSetLayout;

        if (vkAllocateDescriptorSets(seDevice.device(), &allocInfo, &descriptorSet) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        // Descriptor writes array
        std::vector<VkWriteDescriptorSet> descriptorWrites(3);

        // Diffuse texture descriptor
        VkDescriptorImageInfo diffuseImageInfo{};
        diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        diffuseImageInfo.imageView = seCubemap.getDiffuseImageView();
        diffuseImageInfo.sampler = seCubemap.getDiffuseSampler();

        // Diffuse texture write
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &diffuseImageInfo;

        // Specular texture descriptor
        VkDescriptorImageInfo specularImageInfo{};
        specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        specularImageInfo.imageView = seCubemap.getSpecularImageView();
        specularImageInfo.sampler = seCubemap.getSpecularSampler();

        // Specular texture write
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &specularImageInfo;

        // BRDF texture descriptor
        VkDescriptorImageInfo BRDFImageInfo{};
        BRDFImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        BRDFImageInfo.imageView = seCubemap.getBRDFImageView();
        BRDFImageInfo.sampler = seCubemap.getBRDFSampler();

        // BRDF texture write
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSet;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &BRDFImageInfo;

        // Update only the descriptors that are written (ignoring empty ones)
        vkUpdateDescriptorSets(seDevice.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void PBR::renderGameObjects(
        VkCommandBuffer commandBuffer,
        std::vector<SEGameObject>& gameObjects,
        const SECamera& camera,
        se::SEGameObject& viewerObject,
        int frameIndex) 
    {
        for (auto& obj : gameObjects)
        {
            auto mesh = obj.getMesh();
            if (!mesh) continue;

            se::SimplePushConstantData push{};
            push.color = obj.getColor();
            push.transform = obj.getTransformMat4();

            bind(commandBuffer);

            auto material = obj.getMaterial();
                
            mesh->draw(commandBuffer, material, push, frameIndex);
        }
    }

    void PBR::renderCubeMap(VkCommandBuffer commandBuffer)
    {
        seCubemap.render(commandBuffer);
    }

}