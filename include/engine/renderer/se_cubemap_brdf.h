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

	class SECubemapBRDF
	{
	public:
		SECubemapBRDF(SEDevice& device, SERenderer& renderer);

		~SECubemapBRDF();

		SECubemapBRDF(const SECubemapBRDF &) = delete;
		SECubemapBRDF &operator=(const SECubemapBRDF &) = delete;
		SECubemapBRDF(SECubemapBRDF &&) = delete;
		SECubemapBRDF &operator=(SECubemapBRDF &&) = delete;

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
		void destroyOwnedHandles() noexcept;

		SubMeshBuilder createCubeModel(glm::vec3 offset);

		SEDevice& seDevice;
		SERenderer& seRenderer;

		VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
		VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
		std::unique_ptr<SEPipeline> sePipeline;
		VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

		VkImage BRDFImage{VK_NULL_HANDLE};
		VkImageView BRDFImageView{VK_NULL_HANDLE};
		VkDeviceMemory BRDFImageMemory{VK_NULL_HANDLE};
		VkSampler BRDFSampler{VK_NULL_HANDLE};

		std::unique_ptr<SESubMesh> cubeMesh;
	};
}


