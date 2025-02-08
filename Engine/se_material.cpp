#include "se_material.hpp"

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

	SEMaterial::SEMaterial(
		SEDevice &device,
		VkRenderPass renderPass,
		const std::string &vertFilepath,
		const std::string &fragFilepath,
		const VkSampleCountFlagBits msaaSamples,
		float metallic,
		float roughness,
		float ao,
		std::optional<std::shared_ptr<SETexture>> diffuseTexture,
		std::optional<std::shared_ptr<SETexture>> normalTexture,
		std::optional<std::shared_ptr<SETexture>> metallicTexture,
		std::optional<std::shared_ptr<SETexture>> roughnessTexture,
		std::optional<std::shared_ptr<SETexture>> aoTexture)
		: seDevice{device}, renderPass{renderPass}, diffuseTexture{diffuseTexture},
		  normalTexture{normalTexture}, metallicTexture{metallicTexture},
		  roughnessTexture{roughnessTexture}, aoTexture{aoTexture}
	{
		flags.hasDiffuseMap = diffuseTexture.has_value();
		flags.hasNormalMap = normalTexture.has_value();
		flags.hasMetallicMap = metallicTexture.has_value();
		flags.hasRoughnessMap = roughnessTexture.has_value();
		flags.hasAOMap = aoTexture.has_value();

		flags.metallic = metallic;
		flags.roughness = roughness;
		flags.ao = ao;

		dummyTexture = std::make_unique<se::SETexture>(seDevice, "textures/dummy.jpg");
		createUniformBuffer();

		createDescriptorSetLayout();
		createDescriptorSets();
		createPipelineLayout();
		createPipeline(renderPass, vertFilepath, fragFilepath);
	}

	void SEMaterial::createUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(MaterialFlags);

		// Create the buffer (assuming seDevice.createBuffer handles all buffer creation tasks)
		seDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			matBuffer,
			matBufferMemory);

		// Map the buffer memory
		VkResult result = vkMapMemory(seDevice.device(), matBufferMemory, 0, bufferSize, 0, &matBufferMapped);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to map uniform buffer memory!");
		}

		// Copy data to the mapped memory
		memcpy(matBufferMapped, &flags, sizeof(flags));

		// No need to flush for coherent memory, but if non-coherent memory is used:
		// VkMappedMemoryRange memoryRange{};
		// memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		// memoryRange.memory = matBufferMemory;
		// memoryRange.offset = 0;
		// memoryRange.size = VK_WHOLE_SIZE;
		// vkFlushMappedMemoryRanges(seDevice.device(), 1, &memoryRange);
	}

	void SEMaterial::createPipelineLayout()
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(seDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void SEMaterial::createPipeline(VkRenderPass renderPass, const std::string vertPath, const std::string fragPath)
	{
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		SEPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		sePipeline = std::make_unique<SEPipeline>(
			seDevice,
			vertPath,
			fragPath,
			pipelineConfig,
			VK_SAMPLE_COUNT_1_BIT);
	}

	void SEMaterial::createDescriptorSetLayout()
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
		if (vkCreateDescriptorSetLayout(seDevice.device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void SEMaterial::createDescriptorSets()
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = seDevice.getDescriptorPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(seDevice.device(), &allocInfo, &descriptorSet) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		// Uniform buffer descriptor UBO
		// glm::mat4 view;
		// glm::mat4 proj;
		// glm::vec3 cameraPos;
		std::vector<VkBuffer> uniformBuffers = seDevice.getUniformBuffers();
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[0];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		// Uniform buffer descriptor ( Material )
		VkDescriptorBufferInfo matBufferInfo{};
		matBufferInfo.buffer = matBuffer;
		matBufferInfo.offset = 0;
		matBufferInfo.range = sizeof(MaterialFlags);

		// Diffuse texture descriptor
		VkDescriptorImageInfo dummyImageInfo{};
		dummyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		dummyImageInfo.imageView = dummyTexture->getTextureImageView();
		dummyImageInfo.sampler = dummyTexture->getTextureSampler();

		// Descriptor writes array
		std::vector<VkWriteDescriptorSet> descriptorWrites(7);

		// Uniform buffer write
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		// Uniform buffer write ( Material )
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &matBufferInfo;

		// Diffuse texture descriptor
		VkDescriptorImageInfo diffuseImageInfo{};
		if (flags.hasDiffuseMap)
		{
			diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			diffuseImageInfo.imageView = diffuseTexture.value()->getTextureImageView();
			diffuseImageInfo.sampler = diffuseTexture.value()->getTextureSampler();
		}
		// Diffuse texture write
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSet;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = flags.hasDiffuseMap ? &diffuseImageInfo : &dummyImageInfo;

		// Optional textures: Normal, Metallic, Roughness, AO

		VkDescriptorImageInfo normalImageInfo{};
		if (flags.hasNormalMap)
		{
			normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			normalImageInfo.imageView = normalTexture.value()->getTextureImageView();
			normalImageInfo.sampler = normalTexture.value()->getTextureSampler();
		}

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSet;
		descriptorWrites[3].dstBinding = 3; // Normal map binding
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = flags.hasNormalMap ? &normalImageInfo : &dummyImageInfo;

		VkDescriptorImageInfo metallicImageInfo{};
		if (flags.hasMetallicMap)
		{
			metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			metallicImageInfo.imageView = metallicTexture.value()->getTextureImageView();
			metallicImageInfo.sampler = metallicTexture.value()->getTextureSampler();
		}
		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = descriptorSet;
		descriptorWrites[4].dstBinding = 4; // Metallic map binding
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pImageInfo = flags.hasMetallicMap ? &metallicImageInfo : &dummyImageInfo;

		VkDescriptorImageInfo roughnessImageInfo{};
		if (flags.hasRoughnessMap)
		{
			roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			roughnessImageInfo.imageView = roughnessTexture.value()->getTextureImageView();
			roughnessImageInfo.sampler = roughnessTexture.value()->getTextureSampler();
		}
		descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[5].dstSet = descriptorSet;
		descriptorWrites[5].dstBinding = 5; // Roughness map binding
		descriptorWrites[5].dstArrayElement = 0;
		descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[5].descriptorCount = 1;
		descriptorWrites[5].pImageInfo = flags.hasRoughnessMap ? &roughnessImageInfo : &dummyImageInfo;

		VkDescriptorImageInfo aoImageInfo{};
		if (flags.hasAOMap)
		{
			aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			aoImageInfo.imageView = aoTexture.value()->getTextureImageView();
			aoImageInfo.sampler = aoTexture.value()->getTextureSampler();
		}

		descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[6].dstSet = descriptorSet;
		descriptorWrites[6].dstBinding = 6; // AO map binding
		descriptorWrites[6].dstArrayElement = 0;
		descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[6].descriptorCount = 1;
		descriptorWrites[6].pImageInfo = flags.hasAOMap ? &aoImageInfo : &dummyImageInfo;

		// Update only the descriptors that are written (ignoring empty ones)
		vkUpdateDescriptorSets(seDevice.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

}