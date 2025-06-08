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
	};
	
	class SEMaterial : public SEMaterialBase, public Resource
	{

	public:
		struct alignas(16) MaterialFlags
		{
			alignas(16) glm::vec3 color;
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

		void bind(VkCommandBuffer commandBuffer, int frameIndex) override
		{
			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1,  // Set 1 (Material Textures)
				1,
				&descriptorSets[frameIndex],
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

		VkDescriptorSet getDescriptorSet(int frameIndex) const override {
			return descriptorSets[frameIndex];
		}

		VkDescriptorSetLayout getDescriptorSetLayout() const override {
			return descriptorSetLayout;
		}

		const MaterialFlags& getFlags() const
		{
			return flags;
		}

		glm::vec3 getColor() const
		{
			return flags.color;
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
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}

		void setNormalTexture(std::shared_ptr<SETexture> texture)
		{
			normalTexture = texture;
			flags.hasNormalMap = (texture != dummyTexture);
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}

		void setMetallicTexture(std::shared_ptr<SETexture> texture)
		{
			metallicTexture = texture;
			flags.hasMetallicMap = (texture != dummyTexture);
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}

		void setRoughnessTexture(std::shared_ptr<SETexture> texture)
		{
			roughnessTexture = texture;
			flags.hasRoughnessMap = (texture != dummyTexture);
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}

		void setAOTexture(std::shared_ptr<SETexture> texture)
		{
			aoTexture = texture;
			flags.hasAOMap = (texture != dummyTexture);
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}

		void setMetallic(float metallic)
		{
			flags.metallic = metallic;
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}

		void setRoughness(float roughness)
		{
			flags.roughness = roughness;
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}
		
		void setAO(float ao)
		{
			flags.ao = ao;
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}

		void setColor(const glm::vec3& color)
		{
			flags.color = color;
			std::fill(needUpdate.begin(), needUpdate.end(), true);
		}

		void update(int frameIndex)
		{
			if (needUpdate[frameIndex])
			{
				updateDescriptorSet(frameIndex);
				needUpdate[frameIndex] = false;
			}
		}

	private:
		void createUniformBuffer();
		void createDescriptorSets();

		void updateDescriptorSet(size_t frameIndex);

		std::vector<bool> needUpdate;


		SEDevice &seDevice;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSetLayout descriptorSetLayout;
		std::vector<VkDescriptorSet> descriptorSets;

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
