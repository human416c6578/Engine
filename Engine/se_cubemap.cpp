#include "se_cubemap.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace se
{
	SECubemap::SECubemap(SEDevice& device, VkRenderPass renderPass, const std::string& vertFilepath, const std::string& fragFilepath, const std::string& path) : seDevice{device}, renderPass{renderPass}
	{
		mapTexture = std::make_unique<se::SETexture>(seDevice, path);
		se::SESubMesh::Builder builder = createCubeModel({ 0, 0, 0 });
		cubeMesh = std::make_unique<se::SESubMesh>(seDevice, builder);

		createDescriptorSetLayout();
		createDescriptorSets();
		createPipelineLayout();
		createPipeline(renderPass, vertFilepath, fragFilepath);

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
		imageInfo.imageView = mapTexture->getTextureImageView();
		imageInfo.sampler = mapTexture->getTextureSampler();

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

		// Diffuse (Albedo) texture binding (always present)
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

	void SECubemap::render(VkCommandBuffer commandBuffer) {
		draw(commandBuffer);
	}

	void SECubemap::draw(VkCommandBuffer commandBuffer) {
		bind(commandBuffer);
		cubeMesh->bind(commandBuffer);
		cubeMesh->draw(commandBuffer);
	}

	void SECubemap::saveImageToFile(const std::string& filename, VkDevice device, VkDeviceMemory bufferMemory, uint32_t width, uint32_t height) {
		void* data;
		vkMapMemory(device, bufferMemory, 0, VK_WHOLE_SIZE, 0, &data);

		// Save the image using stb_image_write or custom function
		stbi_write_png(filename.c_str(), width, height, 4, data, width * 4); // Assuming RGBA8 format


		vkUnmapMemory(device, bufferMemory);
	}

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

