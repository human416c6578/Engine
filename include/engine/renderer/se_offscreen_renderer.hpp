#pragma once
#include "engine/core/se_device.hpp"
#include <vector>

namespace se
{
	class SEOffscreenRenderer
	{
	public:
		SEOffscreenRenderer(SEDevice& device)
			: seDevice(device) {
			createOffscreenImageDepth();
			createOffscreenImageColor();
			createRenderPass();
			createFramebuffers();
			createCommandBuffers();
			createStagingBuffer();
		
		}

		~SEOffscreenRenderer();

		SEOffscreenRenderer(const SEOffscreenRenderer&) = delete;
		SEOffscreenRenderer& operator=(const SEOffscreenRenderer&) = delete;

		void resize(uint32_t newWidth, uint32_t newHeight);
		void setImageFormat(VkFormat format);

		size_t getCurrentFrame() { return currentFrame; }
		
		VkCommandBuffer getCommandBuffer() { return commandBuffers[currentFrame]; }

		VkRenderPass getRenderPass() { return renderPass; }

		VkFramebuffer getFramebuffer(int frameIndex)
		{
			return framebuffers[frameIndex];
		}

		bool isFrameStarted()
		{
			return frameStarted;
		}

		VkImage getDepthImage(int frameIndex)
		{
			return depthImages[frameIndex];
		}

		VkImage getColorImage(int frameIndex)
		{
			return colorImages[frameIndex];
		}

		VkImageView getDepthImageView(int frameIndex)
		{
			return depthImageViews[frameIndex];
		}

		VkImageView getColorImageView(int frameIndex)
		{
			return colorImageViews[frameIndex];
		}

		uint32_t getWidth() { return width; }
		uint32_t getHeight() { return height; }

		VkBuffer getStagingBuffer() { return stagingBuffer; }
		VkDeviceMemory getStagingBufferMemory() { return stagingBufferMemory; }

		VkCommandBuffer beginFrame();
		void endFrame();

		void beginRenderPass(VkCommandBuffer commandBuffer);
		void endRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createOffscreenImageDepth();
		void createOffscreenImageColor();

		void createRenderPass();
		void createFramebuffers();

		void createStagingBuffer();
		void createCommandBuffers();
		void freeCommandBuffers();

	private:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 3;
		bool frameStarted{ false };
		size_t currentFrame{0};

		SEDevice& seDevice;

		uint32_t width{ 512 };
		uint32_t height{ 512 };
		VkFormat colorFormat{ VK_FORMAT_R8G8B8A8_SRGB };

		std::vector<VkCommandBuffer> commandBuffers;

		VkRenderPass renderPass;

		std::vector<VkFramebuffer> framebuffers;

		std::vector<VkImage> depthImages;
		std::vector<VkDeviceMemory> depthImageMemory;
		std::vector<VkImageView> depthImageViews;

		std::vector<VkImage> colorImages;
		std::vector<VkDeviceMemory> colorImageMemory;
		std::vector<VkImageView> colorImageViews;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
	};
}