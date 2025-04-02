#pragma once
#include "se_device.hpp"

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
			createFramebuffer();
			createCommandBuffers();
			createStagingBuffer();
		
		}

		~SEOffscreenRenderer();

		SEOffscreenRenderer(const SEOffscreenRenderer&) = delete;
		SEOffscreenRenderer& operator=(const SEOffscreenRenderer&) = delete;

		void resize(uint32_t newWidth, uint32_t newHeight);
		void setImageFormat(VkFormat format);

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
		uint32_t width{ 512 };
		uint32_t height{ 512 };
		VkFormat colorFormat{ VK_FORMAT_R8G8B8A8_SRGB };


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
