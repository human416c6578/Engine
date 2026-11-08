#include "engine/renderer/se_postprocess.hpp"
#include "engine/renderer/se_pipeline.hpp"
#include "engine/vulkan/se_device.hpp"
#include "engine/vulkan/se_vk_result.hpp"

#include <cmath>

#include <stdexcept>
#include <array>
#include <cstddef>
#include <cstring>

namespace se
{

	SEPostProcess::SEPostProcess(SEDevice& device, VkRenderPass renderPass)
		: seDevice{ device }, renderPass{ renderPass }
	{
		try
		{
			createPostProcessBuffer();
			createDescriptorSetLayout();
			createPipelineLayout();
			createPipeline();
			createDescriptorSets();
			createTextureSampler();
		}
		catch (...)
		{
			destroyOwnedHandles();
			throw;
		}
	}

	SEPostProcess::~SEPostProcess()
	{
		destroyOwnedHandles();
	}

	void SEPostProcess::destroyOwnedHandles() noexcept
	{
		for (VkDescriptorSet &descriptorSet : descriptorSets)
		{
			if (descriptorSet != VK_NULL_HANDLE)
			{
				seDevice.logResult(vkFreeDescriptorSets(seDevice.device(), seDevice.getDescriptorPool(), 1, &descriptorSet),
					"vkFreeDescriptorSets(SEPostProcess)");
				descriptorSet = VK_NULL_HANDLE;
			}
		}
		descriptorSets.clear();
		if (textureSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(seDevice.device(), textureSampler, nullptr);
			textureSampler = VK_NULL_HANDLE;
		}
		pipeline.reset();
		if (pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(seDevice.device(), pipelineLayout, nullptr);
			pipelineLayout = VK_NULL_HANDLE;
		}
		if (descriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(seDevice.device(), descriptorSetLayout, nullptr);
			descriptorSetLayout = VK_NULL_HANDLE;
		}
		for (std::size_t i = 0; i < dofBuffers.size(); ++i)
		{
			if (dofBuffers[i] != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(seDevice.device(), dofBuffers[i], nullptr);
				dofBuffers[i] = VK_NULL_HANDLE;
			}
			if (i < dofBufferMemories.size() && dofBufferMemories[i] != VK_NULL_HANDLE)
			{
				vkFreeMemory(seDevice.device(), dofBufferMemories[i], nullptr);
				dofBufferMemories[i] = VK_NULL_HANDLE;
			}
		}
		dofBuffers.clear();
		dofBufferMemories.clear();
	}

	void SEPostProcess::createPipelineLayout()
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

		checkVkResult(vkCreatePipelineLayout(
				seDevice.device(),
				&pipelineLayoutInfo,
				nullptr,
				&pipelineLayout), "vkCreatePipelineLayout(postprocess)");
	}

	void SEPostProcess::createPipeline()
	{
		PipelineConfigInfo config{};
		SEPipeline::defaultPipelineConfigInfo(config);

		config.renderPass = renderPass;
		config.pipelineLayout = pipelineLayout;

		config.depthStencilInfo.depthTestEnable = VK_FALSE;
		config.depthStencilInfo.depthWriteEnable = VK_FALSE;

		config.bindingDescriptions.clear();
		config.attributeDescriptions.clear();

		pipeline = std::make_unique<SEPipeline>(
			seDevice,
			"shaders/postprocessVert.spv",
			"shaders/postprocessFrag.spv",
			config,
			VK_SAMPLE_COUNT_1_BIT);
	}

	void SEPostProcess::createDescriptorSets()
	{
		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (std::size_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex)
		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = seDevice.getDescriptorPool();
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;
			checkVkResult(vkAllocateDescriptorSets(seDevice.device(), &allocInfo,
				&descriptorSets[frameIndex]), "vkAllocateDescriptorSets(postprocess)");
		}
	}

	void SEPostProcess::createDescriptorSetLayout()
	{
		std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings[2].binding = 2;
		bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[2].descriptorCount = 1;
		bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		checkVkResult(vkCreateDescriptorSetLayout(
				seDevice.device(),
				&layoutInfo,
				nullptr,
				&descriptorSetLayout), "vkCreateDescriptorSetLayout(postprocess)");
	}

	void SEPostProcess::createPostProcessBuffer()
	{
		const VkDeviceSize bufferSize = sizeof(PostProcessSettings);
		dofBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		dofBufferMemories.resize(MAX_FRAMES_IN_FLIGHT);
		for (std::size_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex)
		{
			seDevice.createBuffer(bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				dofBuffers[frameIndex], dofBufferMemories[frameIndex]);
		}
	}

	void SEPostProcess::updatePostProcessBuffer(std::size_t frameIndex)
	{
		void* data = nullptr;
		checkVkResult(vkMapMemory(seDevice.device(), dofBufferMemories.at(frameIndex),
			0, sizeof(PostProcessSettings), 0, &data), "vkMapMemory(postprocess settings)");
		std::memcpy(data, &postProcessSettings, sizeof(PostProcessSettings));
		vkUnmapMemory(seDevice.device(), dofBufferMemories[frameIndex]);
	}

	void SEPostProcess::updateFrameData(std::size_t frameIndex)
	{
		updatePostProcessBuffer(frameIndex);
	}

	void SEPostProcess::setInput(std::size_t frameIndex, VkImageView colorView, VkImageView depthView)
	{
		VkDescriptorImageInfo colorInfo{};
		colorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colorInfo.imageView = colorView;
		colorInfo.sampler = textureSampler;

		VkDescriptorImageInfo depthInfo{};
		depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		depthInfo.imageView = depthView;
		depthInfo.sampler = textureSampler;

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = dofBuffers.at(frameIndex);
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(PostProcessSettings);

		std::array<VkWriteDescriptorSet, 3> writes{};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = descriptorSets.at(frameIndex);
		writes[0].dstBinding = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorCount = 1;
		writes[0].pImageInfo = &colorInfo;
		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSets.at(frameIndex);
		writes[1].dstBinding = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[1].descriptorCount = 1;
		writes[1].pImageInfo = &depthInfo;
		writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[2].dstSet = descriptorSets.at(frameIndex);
		writes[2].dstBinding = 2;
		writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[2].descriptorCount = 1;
		writes[2].pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(seDevice.device(), static_cast<uint32_t>(writes.size()),
			writes.data(), 0, nullptr);
	}

	void SEPostProcess::createTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		checkVkResult(vkCreateSampler(seDevice.device(), &samplerInfo, nullptr, &textureSampler),
			"vkCreateSampler(postprocess)");
	}

	void SEPostProcess::render(VkCommandBuffer cmd, std::size_t frameIndex)
	{
		pipeline->bind(cmd);

		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&descriptorSets.at(frameIndex),
			0,
			nullptr);

		// Fullscreen triangle
		vkCmdDraw(cmd, 3, 1, 0, 0);
	}

} // namespace se
