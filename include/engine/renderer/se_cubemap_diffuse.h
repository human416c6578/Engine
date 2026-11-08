#pragma once
#include <vulkan/vulkan.h> // Vulkan handle types appear in this interface.
#include <glm/glm.hpp> // glm::vec3 is passed by value.
#include <memory>
#include <string>
#include "engine/renderer/se_pipeline.hpp" // bind() invokes SEPipeline inline.



namespace se
{
	class SEDevice;
	class SERenderer;
	class SESubMesh;
	class SETexture;
	struct SubMeshBuilder;

	class SECubemapDiffuse
	{
	public:
		SECubemapDiffuse(SEDevice& device, SERenderer& renderer, VkImageView cubemapView, VkSampler cubemapSampler);

		~SECubemapDiffuse();

		SECubemapDiffuse(const SECubemapDiffuse &) = delete;
		SECubemapDiffuse &operator=(const SECubemapDiffuse &) = delete;
		SECubemapDiffuse(SECubemapDiffuse &&) = delete;
		SECubemapDiffuse &operator=(SECubemapDiffuse &&) = delete;

		VkSampler getSampler() { return irradianceSampler; }
		VkImage getImage() { return irradianceImage; }
		VkImageView getImageView() { return irradianceImageView; }
		VkDeviceMemory getImageMemory() { return irradianceImageMemory; }

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

		SubMeshBuilder createCubeModel(glm::vec3 offset);

		SEDevice& seDevice;
		SERenderer& seRenderer;

		VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
		VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
		std::unique_ptr<SEPipeline> sePipeline;
		VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

		VkImage irradianceImage{VK_NULL_HANDLE};
		VkImageView irradianceImageView{VK_NULL_HANDLE};
		VkDeviceMemory irradianceImageMemory{VK_NULL_HANDLE};
		VkSampler irradianceSampler{VK_NULL_HANDLE};

		VkImageView cubeMapImageView{VK_NULL_HANDLE}; // borrowed from the HDR converter
		VkSampler cubeMapSampler{VK_NULL_HANDLE}; // borrowed from the HDR converter

		std::unique_ptr<SESubMesh> cubeMesh;
	};
}


