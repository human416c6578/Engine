#include "engine/renderer/se_pbr_material.hpp"
#include "engine/vulkan/se_device.hpp"
#include "engine/vulkan/se_deferred_disposal.hpp"
#include "engine/renderer/se_texture.hpp"
#include "engine/core/se_camera.hpp"
#include "engine/core/se_frame_config.hpp"
#include "engine/renderer/se_swap_chain.hpp"
#include "engine/vulkan/se_vk_result.hpp"
#include <cstring>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>
#include <utility>

namespace se
{
	SEMaterial::SEMaterial(
		SEDevice &device,
		VkPipelineLayout pipelineLayout,
		VkPipeline pipeline,
		VkDescriptorSetLayout descSetLayout,
		const VkSampleCountFlagBits msaaSamples,
		const std::string guid, const std::string name,
		float metallic,
		float roughness,
		float ao,
		std::shared_ptr<SETexture> dummyTexture,
		std::optional<std::shared_ptr<SETexture>> diffuseTexture,
		std::optional<std::shared_ptr<SETexture>> normalTexture,
		std::optional<std::shared_ptr<SETexture>> metallicTexture,
		std::optional<std::shared_ptr<SETexture>> roughnessTexture,
		std::optional<std::shared_ptr<SETexture>> aoTexture,
        std::optional<std::shared_ptr<SETexture>> heightTexture)
		: Resource(guid, name), seDevice{ device }, disposalRecord{device.makeDisposalRecord()},
		pipelineLayout{ pipelineLayout }, pipeline{ pipeline },
		descriptorSetLayout {descSetLayout}, dummyTexture{ dummyTexture }, diffuseTexture{diffuseTexture},
		  normalTexture{normalTexture}, metallicTexture{metallicTexture},
		  roughnessTexture{roughnessTexture}, aoTexture{aoTexture}, heightTexture{heightTexture}
	{
		disposalRecord->payload.emplace<DescriptorBufferDisposal>();
        flags.color = { 1.0f, 1.0f, 1.0f }; // Default color
		flags.hasDiffuseMap = diffuseTexture.has_value();
		flags.hasNormalMap = normalTexture.has_value();
		flags.hasMetallicMap = metallicTexture.has_value();
		flags.hasRoughnessMap = roughnessTexture.has_value();
		flags.hasAOMap = aoTexture.has_value();
        flags.hasHeightMap = heightTexture.has_value();

		flags.metallic = metallic;
		flags.roughness = roughness;
		flags.ao = ao;

        flags.uvScaleX = 1.0;
        flags.uvScaleY = 1.0;
		
		try
		{
			createUniformBuffer();
			createDescriptorSets();
		}
		catch (...)
		{
			destroyOwnedHandles();
			throw;
		}
	}

	SEMaterial::~SEMaterial()
	{
		destroyOwnedHandles();
	}

	void SEMaterial::destroyOwnedHandles() noexcept
	{
		if (!disposalRecord) return; // Already transferred, including constructor rollback.

		auto& payload = std::get<DescriptorBufferDisposal>(disposalRecord->payload);
		payload.pool = seDevice.getDescriptorPool(); // Borrowed; never retire the pool here.
		assert(descriptorSets.size() <= payload.sets.size());
		bool ownsHandles = false;
		for (size_t i = 0; i < descriptorSets.size(); ++i)
		{
			payload.sets[i] = std::exchange(descriptorSets[i], VK_NULL_HANDLE);
			ownsHandles |= payload.sets[i] != VK_NULL_HANDLE;
		}
		descriptorSets.clear();
		for (size_t i = 0; i < matBuffers.size(); ++i)
		{
			// Only owned flags allocations; camera buffers and pipeline/layouts are borrowed.
			auto& allocation = payload.allocations[i];
			allocation = {std::exchange(matBuffers[i], VK_NULL_HANDLE),
				std::exchange(matBufferMemories[i], VK_NULL_HANDLE), matBuffersMapped[i] != nullptr};
			matBuffersMapped[i] = nullptr;
			ownsHandles |= allocation.buffer != VK_NULL_HANDLE || allocation.memory != VK_NULL_HANDLE;
		}
		if (ownsHandles)
			seDevice.retire(std::move(disposalRecord));
		else
			disposalRecord.reset(); // Early failure acquired no Vulkan ownership.
	}

	void SEMaterial::createUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(MaterialFlags);

		for (size_t i = 0; i < matBuffers.size(); ++i)
		{
			seDevice.createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				matBuffers[i],
				matBufferMemories[i]);

			checkVkResult(vkMapMemory(seDevice.device(), matBufferMemories[i], 0, bufferSize, 0, &matBuffersMapped[i]),
				"vkMapMemory(material buffer)");

			// Initialize before use; coherent memory needs no explicit flush.
			memcpy(matBuffersMapped[i], &flags, sizeof(flags));
		}
	}
	void SEMaterial::createDescriptorSets()
	{
		size_t framesInFlight = MAX_FRAMES_IN_FLIGHT;
		descriptorSets.resize(framesInFlight);
		needUpdate.resize(framesInFlight, false);

		for (size_t i = 0; i < framesInFlight; i++)
		{
			if (descriptorSets[i] != VK_NULL_HANDLE)
			{
				seDevice.logResult(vkFreeDescriptorSets(seDevice.device(), seDevice.getDescriptorPool(), 1, &descriptorSets[i]),
					"vkFreeDescriptorSets(SEMaterial frame)");
			}

			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = seDevice.getDescriptorPool();
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;

			checkVkResult(vkAllocateDescriptorSets(seDevice.device(), &allocInfo, &descriptorSets[i]),
				"vkAllocateDescriptorSets(material)");

			// Update descriptor set right after allocation with default/static data if needed
			updateDescriptorSet(i);
		}
	}

    void SEMaterial::updateDescriptorSet(size_t frameIndex)
    {
        // Runtime callers have waited for this frame slot's fence before updating it.
        memcpy(matBuffersMapped[frameIndex], &flags, sizeof(flags));

        std::vector<VkBuffer> uniformBuffers = seDevice.getUniformBuffers();
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[frameIndex];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorBufferInfo matBufferInfo{};
        matBufferInfo.buffer = matBuffers[frameIndex];
        matBufferInfo.offset = 0;
        matBufferInfo.range = sizeof(MaterialFlags);

        VkDescriptorImageInfo dummyImageInfo{};
        dummyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        dummyImageInfo.imageView = dummyTexture->getTextureImageView();
        dummyImageInfo.sampler = dummyTexture->getTextureSampler();

        // Setup image infos for textures with fallback to dummy textures
        VkDescriptorImageInfo diffuseImageInfo{};
        if (flags.hasDiffuseMap)
        {
            diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            diffuseImageInfo.imageView = diffuseTexture.value()->getTextureImageView();
            diffuseImageInfo.sampler = diffuseTexture.value()->getTextureSampler();
        }

        VkDescriptorImageInfo normalImageInfo{};
        if (flags.hasNormalMap)
        {
            normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalImageInfo.imageView = normalTexture.value()->getTextureImageView();
            normalImageInfo.sampler = normalTexture.value()->getTextureSampler();
        }

        VkDescriptorImageInfo metallicImageInfo{};
        if (flags.hasMetallicMap)
        {
            metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallicImageInfo.imageView = metallicTexture.value()->getTextureImageView();
            metallicImageInfo.sampler = metallicTexture.value()->getTextureSampler();
        }

        VkDescriptorImageInfo roughnessImageInfo{};
        if (flags.hasRoughnessMap)
        {
            roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            roughnessImageInfo.imageView = roughnessTexture.value()->getTextureImageView();
            roughnessImageInfo.sampler = roughnessTexture.value()->getTextureSampler();
        }

        VkDescriptorImageInfo aoImageInfo{};
        if (flags.hasAOMap)
        {
            aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            aoImageInfo.imageView = aoTexture.value()->getTextureImageView();
            aoImageInfo.sampler = aoTexture.value()->getTextureSampler();
        }

        VkDescriptorImageInfo heightImageInfo{};
        if (flags.hasHeightMap)
        {
            heightImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            heightImageInfo.imageView = heightTexture.value()->getTextureImageView();
            heightImageInfo.sampler = heightTexture.value()->getTextureSampler();
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites(8);

        // Uniform buffer write
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[frameIndex];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // Material buffer write
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[frameIndex];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &matBufferInfo;

        // Diffuse texture write
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[frameIndex];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = flags.hasDiffuseMap ? &diffuseImageInfo : &dummyImageInfo;

        // Normal texture write
        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptorSets[frameIndex];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo = flags.hasNormalMap ? &normalImageInfo : &dummyImageInfo;

        // Metallic texture write
        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = descriptorSets[frameIndex];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pImageInfo = flags.hasMetallicMap ? &metallicImageInfo : &dummyImageInfo;

        // Roughness texture write
        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet = descriptorSets[frameIndex];
        descriptorWrites[5].dstBinding = 5;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pImageInfo = flags.hasRoughnessMap ? &roughnessImageInfo : &dummyImageInfo;

        // AO texture write
        descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[6].dstSet = descriptorSets[frameIndex];
        descriptorWrites[6].dstBinding = 6;
        descriptorWrites[6].dstArrayElement = 0;
        descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[6].descriptorCount = 1;
        descriptorWrites[6].pImageInfo = flags.hasAOMap ? &aoImageInfo : &dummyImageInfo;

        // Height texture write
        descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[7].dstSet = descriptorSets[frameIndex];
        descriptorWrites[7].dstBinding = 7;
        descriptorWrites[7].dstArrayElement = 0;
        descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[7].descriptorCount = 1;
        descriptorWrites[7].pImageInfo = flags.hasHeightMap ? &heightImageInfo : &dummyImageInfo;

        vkUpdateDescriptorSets(seDevice.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
