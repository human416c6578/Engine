#pragma once

#include <stb_image.h>

#include "se_device.hpp"
#include "se_resource.hpp"

// std
#include <math.h>

namespace se
{
	class SETexture : public Resource
	{
	public:
		SETexture(SEDevice& device, const std::string guid, const std::string name, stbi_uc* pixels, int width, int height);
		~SETexture();

		VkSampler getTextureSampler() { return textureSampler; }
		VkImageView getTextureImageView() { return textureImageView; }

	private:
		SEDevice &seDevice;
		std::string path;

		uint32_t mipLevels;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;

		void createTextureImage(stbi_uc* pixels, int width, int height);
		void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
		VkSampleCountFlagBits getMaxUsableSampleCount();
		void createTextureImageView();
		void createTextureSampler();
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	};
}