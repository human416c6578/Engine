#pragma once

#include "se_device.hpp"
#include "se_renderer.hpp"
#include "se_pipeline.hpp"
#include "se_submesh.hpp"

namespace se 
{
	class SEHdrToCubemap
	{
	public:
		SEHdrToCubemap(SEDevice& device, SERenderer& renderPass, const std::string& textureFilepath);
		
		~SEHdrToCubemap()
		{
		}

		VkSampler getSampler() { return cubeMapSampler; }
		VkImageView getImageView() { return cubeMapImageView; }

		void convert();
		

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass, const std::string vertPath, const std::string fragPath);

		void createDescriptorSets();
		void createDescriptorSetLayout();

		void createCubemapImage();

		void bind(VkCommandBuffer commandBuffer)
		{
			sePipeline->bind(commandBuffer);

			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0,
				1,
				&descriptorSet,
				0,
				nullptr);
		}
		void draw(VkCommandBuffer commandBuffer);

		void createSampler();
		void cleanup();

		se::SESubMesh::Builder createCubeModel(glm::vec3 offset);

		SEDevice& seDevice;
		SERenderer& seRenderer;

		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unique_ptr<SEPipeline> sePipeline;
		VkPipelineLayout pipelineLayout;

		VkImage cubeMapImage;
		VkImageView cubeMapImageView;
		VkDeviceMemory cubeMapImageMemory;

		VkSampler cubeMapSampler;

		std::shared_ptr<se::SESubMesh> cubeMesh;
		std::shared_ptr<se::SETexture> mapTexture;
	};
}


