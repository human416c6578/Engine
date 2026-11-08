#pragma once

#include "engine/resources/se_resource.hpp" // SETexture derives from Resource.

#include <vulkan/vulkan.h>

#include <memory>
#include <string>

namespace se
{
    class SEDevice;
	class SETexture : public Resource
	{
	public:
		using ImagePixels = std::unique_ptr<unsigned char, void(*)(void*)>;

		SETexture(SEDevice& device, const std::string guid, const std::string name, ImagePixels pixels, int width, int height);
		~SETexture();

		SETexture(const SETexture &) = delete;
		SETexture &operator=(const SETexture &) = delete;
		SETexture(SETexture &&) = delete;
		SETexture &operator=(SETexture &&) = delete;

		VkSampler getTextureSampler() { return textureSampler; }
		VkImageView getTextureImageView() { return textureImageView; }
		VkImageLayout getTextureImageLayout() { return textureImageLayout; }
		VkDescriptorSet getTextureDescriptorSet() { return textureDescriptorSet; }

	private:
		SEDevice &seDevice;
		std::string path;

		uint32_t mipLevels{0};
		VkImage textureImage{VK_NULL_HANDLE};
		VkDeviceMemory textureImageMemory{VK_NULL_HANDLE};
		VkImageView textureImageView{VK_NULL_HANDLE};
		VkImageLayout textureImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkSampler textureSampler{VK_NULL_HANDLE};

		VkDescriptorSet textureDescriptorSet{VK_NULL_HANDLE};
		void destroyOwnedHandles() noexcept;

		void createTextureImage(ImagePixels pixels, int width, int height);
		void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
		VkSampleCountFlagBits getMaxUsableSampleCount();
		void createTextureImageView();
		void createTextureSampler();
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
		void createTextureDescriptorSet();
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	};
}
