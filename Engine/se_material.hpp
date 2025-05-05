#pragma once

#include "se_device.hpp"
#include "se_pipeline.hpp"
#include "se_texture.hpp"

// std
#include <memory>
#include <vector>

namespace se
{
	struct SimplePushConstantData
	{
		glm::mat4 transform{1.f};
		alignas(16) glm::vec3 color{};
	};
	
	class SEMaterial
	{
	public:
		struct alignas(16) MaterialFlags
		{
			alignas(4) int hasDiffuseMap;	// 4 bytes (aligned to 4 bytes)
			alignas(4) int hasNormalMap;	// 4 bytes (aligned to 4 bytes)
			alignas(4) int hasRoughnessMap; // 4 bytes (aligned to 4 bytes)
			alignas(4) int hasMetallicMap;	// 4 bytes (aligned to 4 bytes)
			alignas(4) int hasAOMap;		// 4 bytes (aligned to 4 bytes)
			alignas(4) float metallic;		// 4 bytes (aligned to 4 bytes)
			alignas(4) float roughness;		// 4 bytes (aligned to 4 bytes)
			alignas(4) float ao;			// 4 bytes (aligned to 4 bytes)

			// Padding to make the structure size a multiple of 16 bytes (4 padding bytes)
			//alignas(4) int padding; // 4 bytes padding
		};

		SEMaterial(SEDevice &device,
				   VkDescriptorSetLayout descriptorSetLayout,
				   const VkSampleCountFlagBits msaaSamples,
				   float metallic = 0.5,
				   float roughness = 0.5,
				   float ao = 1.0,
				   std::optional<std::shared_ptr<SETexture>> diffuseTexture = std::nullopt,
				   std::optional<std::shared_ptr<SETexture>> normalTexture = std::nullopt,
				   std::optional<std::shared_ptr<SETexture>> metallicTexture = std::nullopt,
				   std::optional<std::shared_ptr<SETexture>> roughnessTexture = std::nullopt,
				   std::optional<std::shared_ptr<SETexture>> aoTexture = std::nullopt);

		~SEMaterial()
		{
		}

		SEMaterial(const SEMaterial &) = delete;
		SEMaterial &operator=(const SEMaterial &) = delete;

		void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
		{
			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1,  // Set 1 (Material Textures)
				1,
				&descriptorSet,
				0,
				nullptr);
		}

	private:
		void createUniformBuffer();
		void createDescriptorSets();

		SEDevice &seDevice;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;

		MaterialFlags flags{};

		VkBuffer matBuffer;
		VkDeviceMemory matBufferMemory;
		void *matBufferMapped;

		std::shared_ptr<se::SETexture> dummyTexture;

		std::optional<std::shared_ptr<SETexture>> diffuseTexture;
		std::optional<std::shared_ptr<SETexture>> normalTexture;
		std::optional<std::shared_ptr<SETexture>> metallicTexture;
		std::optional<std::shared_ptr<SETexture>> roughnessTexture;
		std::optional<std::shared_ptr<SETexture>> aoTexture;
	};

}
