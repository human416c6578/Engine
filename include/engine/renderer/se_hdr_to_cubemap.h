#pragma once

#include "engine/vulkan/se_device.hpp"
#include "engine/renderer/se_renderer.hpp"
#include "engine/renderer/se_pipeline.hpp"
#include "engine/renderer/se_submesh.hpp"

namespace se 
{
	class SEHdrToCubemap
	{
	public:
		SEHdrToCubemap(SEDevice& device, SERenderer& renderPass, const std::string& textureFilepath);
		
		~SEHdrToCubemap();

		SEHdrToCubemap(const SEHdrToCubemap &) = delete;
		SEHdrToCubemap &operator=(const SEHdrToCubemap &) = delete;
		SEHdrToCubemap(SEHdrToCubemap &&) = delete;
		SEHdrToCubemap &operator=(SEHdrToCubemap &&) = delete;

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
		void destroyOwnedHandles() noexcept;

		se::SESubMesh::Builder createCubeModel(glm::vec3 offset);

		SEDevice& seDevice;
		SERenderer& seRenderer;

		VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
		VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
		std::unique_ptr<SEPipeline> sePipeline;
		VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

		VkImage cubeMapImage{VK_NULL_HANDLE};
		VkImageView cubeMapImageView{VK_NULL_HANDLE};
		VkDeviceMemory cubeMapImageMemory{VK_NULL_HANDLE};

		VkSampler cubeMapSampler{VK_NULL_HANDLE};

		std::shared_ptr<se::SESubMesh> cubeMesh;
		std::shared_ptr<se::SETexture> mapTexture;
	};
}


