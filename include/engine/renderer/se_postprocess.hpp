#pragma once

#include "engine/core/se_device.hpp"
#include "engine/renderer/se_renderer.hpp"
#include "engine/renderer/se_pipeline.hpp"


// std
#include <memory>
#include <vector>

namespace se
{
	class SEPostProcess
	{
		struct PostProcessSettings
		{
			float focusDistance = 0.1f;
			float focusRange    = 1.1f;
			float blurScale     = 2.0f;
			float maxBlur       = 1.0f;

			int enableFog = 1;
			float fogStart  = 0.0f;
			float fogEnd    = 30.0f;
			float fogIntensity = 0.15f;

			float fogColor[3] = {0.6f, 0.7f, 0.8f};

			int enableTonemap = 1;
			float exposure = 1.0f;
		};

	public:
		SEPostProcess(SEDevice& device, VkRenderPass renderPass);

		void render(VkCommandBuffer cmd);

		void setInput(
			VkImageView colorView,
			VkImageView depthView);
		
		const PostProcessSettings& getSettings() const { return postProcessSettings; }

		void updateSettings(const PostProcessSettings& settings)
		{
			postProcessSettings = settings;
			updatePostProcessBuffer();
		}

	private:
		void createPipeline();
		void createPipelineLayout();
		void createDescriptorSetLayout();
		void createDescriptorSets();
		void createTextureSampler();
		void createPostProcessBuffer();
		void updatePostProcessBuffer();

		SEDevice& seDevice;
		VkRenderPass renderPass;

		VkSampler textureSampler;

		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;

		std::unique_ptr<SEPipeline> pipeline;
		VkPipelineLayout pipelineLayout;

		VkImageView colorImageView;
		VkImageView depthImageView;

		VkBuffer dofBuffer;
		VkDeviceMemory dofBufferMemory;
		PostProcessSettings postProcessSettings;
	};

}