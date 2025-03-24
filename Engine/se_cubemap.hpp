#pragma once

#include "se_device.hpp"
#include "se_renderer.hpp"
#include "se_pipeline.hpp"
#include "se_submesh.hpp"

// std
#include <memory>
#include <vector>

namespace se
{
	class SECubemap
	{
	public:

		SECubemap(SEDevice& device, SERenderer& renderPass,
			const std::string& vertFilepath, const std::string& fragFilepath, const std::string& textureFilepath);

		~SECubemap()
		{
		}

		VkPipelineLayout getPipelineLayout() { return pipelineLayout; };

		SECubemap(const SECubemap&) = delete;
		SECubemap& operator=(const SECubemap&) = delete;

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

		void render(VkCommandBuffer commandBuffer);

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass, const std::string vertPath, const std::string fragPath);

		void createDescriptorSets();
		void createDescriptorSetLayout();

		void draw(VkCommandBuffer commandBuffer);

		void convert();
		void saveImageToFile(const std::string& filename, VkImage image, VkBuffer buffer, VkDeviceMemory bufferMemory, uint32_t width, uint32_t height);

		se::SESubMesh::Builder createCubeModel(glm::vec3 offset);


		SEDevice& seDevice;
		SERenderer& seRenderer;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unique_ptr<SEPipeline> sePipeline;
		VkPipelineLayout pipelineLayout;

		VkImage offscreenImage;
		VkImageView offscreenImageView;
		VkDeviceMemory offscreenImageMemory;
		VkFramebuffer offscreenFramebuffer;

		
		std::unique_ptr<se::SESubMesh> cubeMesh;
		std::shared_ptr<se::SETexture> mapTexture;
	};

}
