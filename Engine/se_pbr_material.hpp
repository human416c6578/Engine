#pragma once

#include "se_device.hpp"
#include "se_pipeline.hpp"
#include "se_texture.hpp"
#include "se_material_base.hpp"

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
	
	class SEMaterial : public SEMaterialBase, public Resource
	{

	public:
		struct alignas(16) MaterialFlags
		{
			alignas(4) int hasDiffuseMap;
			alignas(4) int hasNormalMap;
			alignas(4) int hasRoughnessMap;
			alignas(4) int hasMetallicMap;
			alignas(4) int hasAOMap;
			alignas(4) float metallic;
			alignas(4) float roughness;
			alignas(4) float ao;

			// Padding to make the structure size a multiple of 16 bytes (4 padding bytes)
			//alignas(4) int padding; // 4 bytes padding
		};

		SEMaterial(SEDevice &device,
					VkPipelineLayout pipelineLayout,
					VkPipeline pipeline,
					VkDescriptorSetLayout descriptorSetLayout,
					const VkSampleCountFlagBits msaaSamples,
					const std::string guid, const std::string name,
					float metallic = 0.0,
					float roughness = 0.95,
					float ao = 1.0,
					std::shared_ptr<SETexture> dummyTexture = nullptr,
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

		void bind(VkCommandBuffer commandBuffer) override
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

		Type getType() const override { return Type::PBR; }

		VkPipelineLayout getPipelineLayout() const override {
			return pipelineLayout;
		}

		VkPipeline getPipeline() const override {
			return pipeline;
		}

		VkDescriptorSet getDescriptorSet() const override {
			return descriptorSet;
		}

		VkDescriptorSetLayout getDescriptorSetLayout() const override {
			return descriptorSetLayout;
		}

		const MaterialFlags& getFlags() const
		{
			return flags;
		}

		float getMetallic() const
		{
			return flags.metallic;
		}

		float getRoughness() const
		{
			return flags.roughness;
		}

		float getAO() const
		{
			return flags.ao;
		}

		std::shared_ptr<SETexture> getDiffuseTexture() const
		{
			return diffuseTexture.value_or(dummyTexture);
		}

		std::shared_ptr<SETexture> getNormalTexture() const
		{
			return normalTexture.value_or(dummyTexture);
		}

		std::shared_ptr<SETexture> getMetallicTexture() const
		{
			return metallicTexture.value_or(dummyTexture);
		}

		std::shared_ptr<SETexture> getRoughnessTexture() const
		{
			return roughnessTexture.value_or(dummyTexture);
		}

		std::shared_ptr<SETexture> getAOTexture() const
		{
			return aoTexture.value_or(dummyTexture);
		}

		void setDiffuseTexture(std::shared_ptr<SETexture> texture)
		{
			diffuseTexture = texture;
			flags.hasDiffuseMap = (texture != dummyTexture);
			needUpdate = true;
		}

		void setNormalTexture(std::shared_ptr<SETexture> texture)
		{
			normalTexture = texture;
			flags.hasNormalMap = (texture != dummyTexture);;
			needUpdate = true;;
		}

		void setMetallicTexture(std::shared_ptr<SETexture> texture)
		{
			metallicTexture = texture;
			flags.hasMetallicMap = (texture != dummyTexture);;
			needUpdate = true;
		}

		void setRoughnessTexture(std::shared_ptr<SETexture> texture)
		{
			roughnessTexture = texture;
			flags.hasRoughnessMap = (texture != dummyTexture);;
			needUpdate = true;
		}

		void setAOTexture(std::shared_ptr<SETexture> texture)
		{
			aoTexture = texture;
			flags.hasAOMap = (texture != dummyTexture);;
			needUpdate = true;
		}

		void setMetallic(float metallic)
		{
			flags.metallic = metallic;
			needUpdate = true;
		}

		void setRoughness(float roughness)
		{
			flags.roughness = roughness;
			needUpdate = true;
		}
		
		void setAO(float ao)
		{
			flags.ao = ao;
			needUpdate = true;
		}

		void update()
		{
			if (needUpdate)
			{
				createUniformBuffer();
				createDescriptorSets();
				needUpdate = false;
			}
		}

	private:
		void createUniformBuffer();
		void createDescriptorSets();

		bool needUpdate = false;

		SEDevice &seDevice;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

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
