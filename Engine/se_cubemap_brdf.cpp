#include "se_cubemap_brdf.h"
#include "se_gameobject.hpp"
#include "se_camera.hpp"

namespace se
{
	SECubemapBRDF::SECubemapBRDF(SEDevice& device, SERenderer& renderer) : seDevice{ device }, seRenderer{ renderer }
	{
		se::SESubMesh::Builder builder = createCubeModel({ 0, 0, 0 });
		cubeMesh = std::make_unique<se::SESubMesh>(seDevice, builder);
	}


	void SECubemapBRDF::createPipelineLayout()
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

	void SECubemapBRDF::createPipeline(VkRenderPass renderPass, const std::string vertPath, const std::string fragPath)
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

	void SECubemapBRDF::createDescriptorSets()
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
		std::vector<VkWriteDescriptorSet> descriptorWrites(1);

		// Uniform buffer descriptor UBO
		// glm::mat4 view;
		// glm::mat4 proj;
		// glm::vec3 cameraPos;
		std::vector<VkBuffer> uniformBuffers = seDevice.getUniformBuffers();
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[0];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		// Uniform buffer write
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		// Update only the descriptors that are written (ignoring empty ones)
		vkUpdateDescriptorSets(seDevice.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	void SECubemapBRDF::createDescriptorSetLayout()
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

	void SECubemapBRDF::createBRDFImage()
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.extent.width = 512;
		imageInfo.extent.height = 512;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		//imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		seDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, BRDFImage, BRDFImageMemory);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = BRDFImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(seDevice.device(), &viewInfo, nullptr, &BRDFImageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create cubemap image view!");
		}
	}

	void SECubemapBRDF::draw(VkCommandBuffer commandBuffer)
	{
		bind(commandBuffer);
		cubeMesh->bind(commandBuffer);
		cubeMesh->draw(commandBuffer);
	}

	void SECubemapBRDF::generate()
	{
		se::SEOffscreenRenderer* offscreenRenderer = seRenderer.getOffscreenRenderer();

		offscreenRenderer->resize(512, 512);
		offscreenRenderer->setImageFormat(VK_FORMAT_R8G8B8A8_SRGB);

		createDescriptorSetLayout();
		createDescriptorSets();
		createPipelineLayout();
		createPipeline(seRenderer.getOffscreenRenderer()->getRenderPass(), "shaders/BRDFVert.spv", "shaders/BRDFFrag.spv");
		createBRDFImage();

		se::SECamera camera{};
		auto viewerObject = se::SEGameObject::createGameObject();
		
		float aspect = 1.0f; // Square images for cubemap faces
		camera.setPerspectiveProjection(glm::radians(90.f), aspect, 0.01f, 1000.f);

		camera.setViewDirection(viewerObject.transform.translation, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }); // Front

		UniformBufferObject ubo{};
		ubo.proj = camera.getProjection();
		ubo.view = camera.getView();
		ubo.cameraPos = viewerObject.transform.translation;

		seDevice.updateUniformBuffers(ubo);
		if (auto commandBuffer = seRenderer.beginOffscreenFrame())
		{
			seRenderer.beginOffscreenRenderPass(commandBuffer);

			draw(commandBuffer);

			seRenderer.endOffscreenRenderPass(commandBuffer);
			seRenderer.endOffscreenFrame();

			// Copy from offscreen render target to cube map image
			VkCommandBuffer cmdBuffer = seDevice.beginSingleTimeCommands();

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = offscreenRenderer->getColorImage();
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			VkImageMemoryBarrier dstBarrier{};
			dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			dstBarrier.image = BRDFImage;
			dstBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			dstBarrier.subresourceRange.baseMipLevel = 0;
			dstBarrier.subresourceRange.levelCount = 1;
			dstBarrier.subresourceRange.baseArrayLayer = 0;
			dstBarrier.subresourceRange.layerCount = 1;
			dstBarrier.srcAccessMask = 0; // No previous access
			dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Preparing for transfer write

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // No previous stages
				VK_PIPELINE_STAGE_TRANSFER_BIT,    // Transfer stage
				0,
				0, nullptr,
				0, nullptr,
				1, &dstBarrier
			);

			VkImageCopy region{};
			region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.srcSubresource.mipLevel = 0;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = 1;
			region.srcOffset = { 0, 0, 0 };
			region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.dstSubresource.mipLevel = 0;
			region.dstSubresource.baseArrayLayer = 0;
			region.dstSubresource.layerCount = 1;
			region.dstOffset = { 0, 0, 0 };
			region.extent = { offscreenRenderer->getWidth(), offscreenRenderer->getHeight(), 1 };


			vkCmdCopyImage(
				cmdBuffer,
				offscreenRenderer->getColorImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				BRDFImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &region
			);


			// Transition the cubemap face to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			VkImageMemoryBarrier cubemapBarrier{};
			cubemapBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			cubemapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			cubemapBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			cubemapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			cubemapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			cubemapBarrier.image = BRDFImage;
			cubemapBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			cubemapBarrier.subresourceRange.baseMipLevel = 0;
			cubemapBarrier.subresourceRange.levelCount = 1;
			cubemapBarrier.subresourceRange.baseArrayLayer = 0;
			cubemapBarrier.subresourceRange.layerCount = 1;
			cubemapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			cubemapBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &cubemapBarrier
			);


			seDevice.endSingleTimeCommands(cmdBuffer);
		}

		createSampler();

		cleanup();
	}

	void SECubemapBRDF::createSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_TRUE;
		//samplerInfo.maxAnisotropy = seDevice.properties.limits.maxSamplerAnisotropy;
		samplerInfo.maxAnisotropy = 4.0;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(0.0f);
		samplerInfo.mipLodBias = 0.0f;

		if (vkCreateSampler(seDevice.device(), &samplerInfo, nullptr, &BRDFSampler) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	void SECubemapBRDF::cleanup()
	{
	}

	se::SESubMesh::Builder SECubemapBRDF::createCubeModel(glm::vec3 offset)
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

