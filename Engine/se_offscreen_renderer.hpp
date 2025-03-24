#pragma once
#include "se_device.hpp"

namespace se
{
	class SEOffscreenRenderer
	{
	public:
		SEOffscreenRenderer(SEDevice& device, uint32_t w, uint32_t h)
			: seDevice(device), width(w), height(h) {
			createOffscreenImageDepth();
			createOffscreenImageColor();
			createRenderPass();
			createFramebuffer();
			createCommandBuffers();
			createStagingBuffer();
		
		}

		~SEOffscreenRenderer();

		VkCommandBuffer getCommandBuffer() { return commandBuffers[0]; }

		VkRenderPass getRenderPass() { return renderPass; }
		VkFramebuffer getFramebuffer() { return framebuffer; }

		uint32_t getWidth() { return width; }
		uint32_t getHeight() { return height; }

		VkImage getDepthImage() { return depthImage; }
		VkImage getColorImage() { return colorImage; }
		VkBuffer getStagingBuffer() { return stagingBuffer; }
		VkDeviceMemory getStagingBufferMemory() { return stagingBufferMemory; }


	private:
		void createOffscreenImageDepth();
		void createOffscreenImageColor();
		void createRenderPass();
		void createFramebuffer();
		void createStagingBuffer();

		void createCommandBuffers();
		void freeCommandBuffers();


		SEDevice& seDevice;
		uint32_t width;
		uint32_t height;

		std::vector<VkCommandBuffer> commandBuffers;

		VkFramebuffer framebuffer;
		VkRenderPass renderPass;

		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;
		VkImage colorImage;
		VkImageView colorImageView;
		VkDeviceMemory colorImageMemory;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
	

	};
}
