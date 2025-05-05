#pragma once

#include "se_device.hpp"
#include "se_renderer.hpp"
#include "se_hdr_to_cubemap.h"
#include "se_cubemap_diffuse.h"
#include "se_cubemap_specular.h"
#include "se_cubemap_brdf.h"


// std
#include <memory>
#include <vector>

namespace se
{
	class SECubemap
	{
	public:

		SECubemap(SEDevice& device, SERenderer& renderPass, const std::string& textureFilepath);

		~SECubemap()
		{

		}

		SECubemap(const SECubemap&) = delete;
		SECubemap& operator=(const SECubemap&) = delete;
		
		void render(VkCommandBuffer commandBuffer)
		{
			bind(commandBuffer);
			draw(commandBuffer);
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

		se::SESubMesh::Builder createCubeModel(glm::vec3 offset);

		SEDevice& seDevice;
		SERenderer& seRenderer;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;

		std::unique_ptr<SEPipeline> sePipeline;
		VkPipelineLayout pipelineLayout;

		std::unique_ptr<SEHdrToCubemap> seCubemapConverter;
		std::unique_ptr<SECubemapSpecular> seSpecular;
		std::unique_ptr<SECubemapDiffuse> seDiffuse;
		std::unique_ptr<SECubemapBRDF> seBRDF;

		std::shared_ptr<se::SESubMesh> cubeMesh;
		std::shared_ptr<se::SETexture> brdfLutTexture;
	};

}