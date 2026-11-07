#pragma once

#include "engine/vulkan/se_device.hpp"
#include "engine/renderer/se_renderer.hpp"
#include "engine/renderer/se_pipeline.hpp"

#include <cstddef>


// std
#include <memory>
#include <vector>

namespace se
{
	class SEPostProcess
	{
	public:
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
		SEPostProcess(SEDevice& device, VkRenderPass renderPass);
		~SEPostProcess();

		void render(VkCommandBuffer cmd, std::size_t frameIndex);

		void setInput(
			std::size_t frameIndex,
			VkImageView colorView,
			VkImageView depthView);
		void updateFrameData(std::size_t frameIndex);

		const PostProcessSettings& getSettings() const { return postProcessSettings; }

		void updateSettings(const PostProcessSettings& settings)
		{
			postProcessSettings = settings;
		}

	private:
		void createPipeline();
		void createPipelineLayout();
		void createDescriptorSetLayout();
		void createDescriptorSets();
		void createTextureSampler();
		void createPostProcessBuffer();
		void updatePostProcessBuffer(std::size_t frameIndex);

		SEDevice& seDevice;
		VkRenderPass renderPass;

		VkSampler textureSampler;

		std::vector<VkDescriptorSet> descriptorSets;
		VkDescriptorSetLayout descriptorSetLayout;

		std::unique_ptr<SEPipeline> pipeline;
		VkPipelineLayout pipelineLayout;

		std::vector<VkBuffer> dofBuffers;
		std::vector<VkDeviceMemory> dofBufferMemories;
		PostProcessSettings postProcessSettings;
	};

}
