#include "engine/renderer/se_texture_system.hpp"
#include "engine/renderer/se_texture.hpp"
#include "engine/vulkan/se_device.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <third_party/stb/stb_image.h>

#if defined(_WIN32)
	#define PATH_SEPARATOR   '\\'
#else
	#define PATH_SEPARATOR   '/'
#endif

namespace se
{
	TextureSystem::TextureSystem(se::SEDevice& device) : seDevice{ device }
	{
		dummyTexture = this->loadTexture("GUID_DUMMY", "DUMMY", "assets/textures/dummy.jpg");
	}

	TextureSystem::~TextureSystem()
	{
	}
	std::shared_ptr<se::SETexture> TextureSystem::loadTexture(const std::string guid, const std::string& name, const std::string& path)
	{

		int width, height, texChannels;

		auto normalize = [](std::string s) {
            std::replace(s.begin(), s.end(), '\\', '/');
            return s;
        };

        std::string finalPath = normalize(path);

		stbi_uc* pixels = stbi_load(finalPath.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
		
		if (!pixels)
		{
			throw std::runtime_error("Failed to load texture image: " + path);
		}
		se::SETexture::ImagePixels pixelOwner{pixels, &stbi_image_free};
		auto texture = std::make_shared<se::SETexture>(seDevice, guid, name, std::move(pixelOwner), width, height);

		textures.insert({ guid, texture });

		//stbi_image_free(pixels);

		return texture;
	}
	
}
