#pragma once
#include <vulkan/vulkan.h> // Vulkan handle types appear in this interface.
#include <glm/glm.hpp> // glm::vec3 is passed by value.
#include <array>
#include "engine/core/se_frame_config.hpp"
#include <memory>
#include <string>
#include "engine/renderer/se_pipeline.hpp" // bind() invokes SEPipeline inline.
#include "engine/renderer/se_cubemap_diffuse.h" // Inline getters call SECubemapDiffuse.
#include "engine/renderer/se_cubemap_specular.h" // Inline getters call SECubemapSpecular.
#include "engine/renderer/se_cubemap_brdf.h" // Inline getters call SECubemapBRDF.




// std

namespace se
{
	class SEDevice;
	class SERenderer;
	class SEHdrToCubemap;
	class SESubMesh;
	class SETexture;
	struct SubMeshBuilder;
	class SECubemap
	{
	public:

		SECubemap(SEDevice& device, SERenderer& renderPass, const std::string& textureFilepath);

		~SECubemap();

		SECubemap(const SECubemap&) = delete;
		SECubemap& operator=(const SECubemap&) = delete;
		SECubemap(SECubemap&&) = delete;
		SECubemap& operator=(SECubemap&&) = delete;
		
		void render(VkCommandBuffer commandBuffer, uint32_t frameIndex)
		{
			bind(commandBuffer, frameIndex);
			draw(commandBuffer, frameIndex);
		}

		void saveImageToFile(const std::string& filename, VkImage colorImage, uint32_t width, uint32_t height);
		
		VkSampler getSpecularSampler() { return seSpecular->getSampler(); }
		VkImageView getSpecularImageView() { return seSpecular->getImageView(); }

		VkSampler getDiffuseSampler() { return seDiffuse->getSampler(); }
		VkImageView getDiffuseImageView() { return seDiffuse->getImageView(); }

		VkSampler getBRDFSampler() { return seBRDF->getSampler(); }
		VkImageView getBRDFImageView() { return seBRDF->getImageView(); }
		//VkSampler getBRDFSampler() { return brdfLutTexture->getTextureSampler(); }
		//VkImageView getBRDFImageView() { return brdfLutTexture->getTextureImageView(); }

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass, const std::string vertPath, const std::string fragPath);
		void createDescriptorSets();
		void createDescriptorSetLayout();
		void destroyOwnedHandles() noexcept;

		void bind(VkCommandBuffer commandBuffer, uint32_t frameIndex)
		{
			sePipeline->bind(commandBuffer);

			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0,
				1,
				&descriptorSets[frameIndex],
				0,
				nullptr);
		}

		void draw(VkCommandBuffer commandBuffer, uint32_t frameIndex);

		SubMeshBuilder createCubeModel(glm::vec3 offset);

		SEDevice& seDevice;
		SERenderer& seRenderer;
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets{};
		VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};

		std::unique_ptr<SEPipeline> sePipeline;
		VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

		std::unique_ptr<SEHdrToCubemap> seCubemapConverter;
		std::unique_ptr<SECubemapSpecular> seSpecular;
		std::unique_ptr<SECubemapDiffuse> seDiffuse;
		std::unique_ptr<SECubemapBRDF> seBRDF;

		std::shared_ptr<se::SESubMesh> cubeMesh;
		std::shared_ptr<se::SETexture> brdfLutTexture;
	};

}
