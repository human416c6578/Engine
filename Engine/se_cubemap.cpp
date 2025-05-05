#include "se_cubemap.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>

namespace se
{
	SECubemap::SECubemap(SEDevice& device, SERenderer& renderer, const std::string& path)
		: seDevice{ device }, seRenderer{ renderer }
	{
		se::SESubMesh::Builder builder = createCubeModel({ 0, 0, 0 });
		cubeMesh = std::make_unique<se::SESubMesh>(seDevice, builder);

		seCubemapConverter = std::make_unique<SEHdrToCubemap>(device, renderer, path);
		
		seCubemapConverter->convert();

		VkSampler sampler = seCubemapConverter->getSampler();
		VkImageView view = seCubemapConverter->getImageView();
		
		seDiffuse = std::make_unique<SECubemapDiffuse>(device, renderer, view, sampler);
		seSpecular = std::make_unique<SECubemapSpecular>(device, renderer, view, sampler);
		seBRDF = std::make_unique<SECubemapBRDF>(device, renderer);

		seDiffuse->convert();
		seSpecular->convert();
		seBRDF->generate();
		//brdfLutTexture = std::make_unique<SETexture>(seDevice, "textures/brdf_lut.png");

		createDescriptorSetLayout();
		createDescriptorSets();
		createPipelineLayout();
		createPipeline(renderer.getSwapChainRenderPass(), "shaders/backgroundVert.spv", "shaders/backgroundFrag.spv");
		
	}

	void SECubemap::createPipelineLayout()
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(seDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void SECubemap::createPipeline(VkRenderPass renderPass, const std::string vertPath, const std::string fragPath)
	{
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		PipelineConfigInfo pipelineConfig{};
		SEPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipelineConfig.depthStencilInfo = depthStencil;
		sePipeline = std::make_unique<SEPipeline>(
			seDevice,
			vertPath,
			fragPath,
			pipelineConfig,
			VK_SAMPLE_COUNT_1_BIT);
	}

	void SECubemap::createDescriptorSets()
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

		// Descriptor writes array
		std::vector<VkWriteDescriptorSet> descriptorWrites(2);

		// Uniform buffer descriptor UBO
		// glm::mat4 view;
		// glm::mat4 proj;
		// glm::vec3 cameraPos;
		std::vector<VkBuffer> uniformBuffers = seDevice.getUniformBuffers();
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[0];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		// Cubemap texture descriptor
		VkDescriptorImageInfo imageInfo{};

		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = seCubemapConverter->getImageView();
		imageInfo.sampler = seCubemapConverter->getSampler();

		// Uniform buffer write
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		// Cubemap texture write
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		// Update only the descriptors that are written (ignoring empty ones)
		vkUpdateDescriptorSets(seDevice.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	void SECubemap::createDescriptorSetLayout()
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

		// Cubemap Texture
		VkDescriptorSetLayoutBinding cubemapLayoutBinding{};
		cubemapLayoutBinding.binding = 1;
		cubemapLayoutBinding.descriptorCount = 1;
		cubemapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		cubemapLayoutBinding.pImmutableSamplers = nullptr;
		cubemapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings.push_back(cubemapLayoutBinding);

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


	void SECubemap::draw(VkCommandBuffer commandBuffer)
	{
		bind(commandBuffer);
		cubeMesh->bind(commandBuffer);
		cubeMesh->draw(commandBuffer);
	}

	/*
	void SECubemap::saveImageToFile(const std::string& filename, VkImage colorImage, uint32_t width, uint32_t height)
	{
		VkDevice device = seDevice.device();
		VkQueue graphicsQueue = seDevice.graphicsQueue();
		VkDeviceSize imageSize = width * height * 4;

		// 1. Create Staging Buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		// Buffer Create Info
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create staging buffer!");
		}

		// Memory Requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = seDevice.findMemoryType(memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// Allocate Memory
		if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate staging buffer memory!");
		}

		// Bind the buffer memory
		vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

		// 2. Copy Image from GPU to Staging Buffer
		VkCommandBuffer commandBuffer = seDevice.beginSingleTimeCommands();

		// Image Layout Transition (Prepare for Copy)
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // Assuming image is in this layout
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;      // Transition to a layout compatible for transfer
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = colorImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		// Define Copy Region
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		// Copy Image to Buffer
		vkCmdCopyImageToBuffer(
			commandBuffer,
			colorImage,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			stagingBuffer,
			1,
			&region
		);

		// Restore Image Layout (After Copy)
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		seDevice.endSingleTimeCommands(commandBuffer);

		// 3. Map Buffer and Copy Data to CPU
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);

		std::vector<uint8_t> imageData(imageSize);
		memcpy(imageData.data(), data, imageSize);

		vkUnmapMemory(device, stagingBufferMemory);

		// 5. Save Image to File
		stbi_flip_vertically_on_write(1);  // Flip image vertically for correct orientation
		if (!stbi_write_png(filename.c_str(), width, height, 4, imageData.data(), width * 4)) {
			throw std::runtime_error("Failed to save image!");
		}

		std::cout << "Image saved to " << filename << std::endl;

		// 6. Cleanup Resources
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	*/


	se::SESubMesh::Builder SECubemap::createCubeModel(glm::vec3 offset)
	{
		se::SESubMesh::Builder modelBuilder{};

		// Define the vertices for the cube. Each face has 4 vertices.
		modelBuilder.vertices = {
			// left face
			{{-.5f, -.5f, -.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
			{{-.5f, .5f, .5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
			{{-.5f, -.5f, .5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
			{{-.5f, .5f, -.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},

			// right face
			{{.5f, -.5f, -.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{.5f, .5f, .5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			{{.5f, -.5f, .5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
			{{.5f, .5f, -.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

			// top face
			{{-.5f, -.5f, -.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{.5f, -.5f, .5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
			{{-.5f, -.5f, .5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
			{{.5f, -.5f, -.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},

			// bottom face
			{{-.5f, .5f, -.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
			{{.5f, .5f, .5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
			{{-.5f, .5f, .5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
			{{.5f, .5f, -.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},

			// nose face
			{{-.5f, -.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
			{{.5f, .5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
			{{-.5f, .5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			{{.5f, -.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},

			// tail face
			{{-.5f, -.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{.5f, .5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
			{{-.5f, .5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
			{{.5f, -.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		};

		// Apply offset to positions
		for (auto& v : modelBuilder.vertices)
		{
			v.position += offset;
		}

		// Define the indices for drawing the cube with triangles
		modelBuilder.indices = {
			0, 1, 2, 0, 3, 1,       // left face
			4, 5, 6, 4, 7, 5,       // right face
			8, 9, 10, 8, 11, 9,     // top face
			12, 13, 14, 12, 15, 13, // bottom face
			16, 17, 18, 16, 19, 17, // nose face
			20, 21, 22, 20, 23, 21  // tail face
		};

		return modelBuilder;
	}

}

