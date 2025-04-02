#pragma once

#include "se_device.hpp"
#include "se_renderer.hpp"
#include "se_pipeline.hpp"
#include "se_submesh.hpp"

namespace se
{
	class SECubemapBRDF
	{
	public:
		SECubemapBRDF(SEDevice& device, SERenderer& renderer);

		~SECubemapBRDF()
		{
		}

		VkSampler getSampler() { return BRDFSampler; }
		VkImage getImage() { return BRDFImage; }
		VkImageView getImageView() { return BRDFImageView; }
		VkDeviceMemory getImageMemory() { return BRDFImageMemory; }

		void generate();


	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass, const std::string vertPath, const std::string fragPath);

		void createDescriptorSets();
		void createDescriptorSetLayout();

		void createBRDFImage();

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

		VkImage BRDFImage;
		VkImageView BRDFImageView;
		VkDeviceMemory BRDFImageMemory;
		VkSampler BRDFSampler;

		std::unique_ptr<se::SESubMesh> cubeMesh;
	};
}


