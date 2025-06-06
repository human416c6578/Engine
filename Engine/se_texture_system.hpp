#pragma once  

#include <filesystem>
#include <string>  
#include <vector>  
#include <unordered_map>  
#include <memory>  

#include "se_device.hpp"
#include "se_texture.hpp"

namespace se
{
	class TextureSystem
	{
	public:
		TextureSystem(se::SEDevice& device);
		~TextureSystem();

		TextureSystem(const TextureSystem&) = delete;
		TextureSystem& operator=(const TextureSystem&) = delete;

		std::unordered_map<std::string, std::shared_ptr<se::SETexture>>* getTextures()
		{
			return &textures;
		}

		std::shared_ptr<se::SETexture> getDummyTexture() const
		{
			return dummyTexture;
		}

		std::shared_ptr<se::SETexture> loadTexture(const std::string guid, const std::string& name, const std::string& path);

	private:
		se::SEDevice& seDevice;
		std::shared_ptr<se::SETexture> dummyTexture;

		std::unordered_map<std::string, std::shared_ptr<se::SETexture>> textures;

	};
} // namespace se