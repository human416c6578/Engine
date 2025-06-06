#include "se_texture_system.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace se
{
	TextureSystem::TextureSystem(se::SEDevice& device) : seDevice{ device }
	{
		dummyTexture = this->loadTexture("GUID_DUMMY", "DUMMY", "textures/dummy.jpg");
	}

	TextureSystem::~TextureSystem()
	{
	}
	std::shared_ptr<se::SETexture> TextureSystem::loadTexture(const std::string guid, const std::string& name, const std::string& path)
	{

		int width, height, texChannels;
		stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
		
		if (!pixels)
		{
			throw std::runtime_error("Failed to load texture image: " + path);
		}
		auto texture = std::make_shared<se::SETexture>(seDevice, guid, name, pixels, width, height);

		textures.insert({ guid, texture });

		//stbi_image_free(pixels);

		return texture;
	}
	
}