#pragma once  

#include "se_pbr_material.hpp"  
#include "se_material_system.hpp"  
#include "se_texture_system.hpp"  
#include "se_mesh_system.hpp"

namespace se  
{  
class ResourceManager  
{  
public:  
	ResourceManager();  
	~ResourceManager();  

	ResourceManager(const ResourceManager&) = delete;  
	ResourceManager& operator=(const ResourceManager&) = delete;

	std::unordered_map<std::string, std::shared_ptr<se::SEMesh>>* getMeshes() const
	{
		if (meshSystem == nullptr)
		{
			throw std::runtime_error("Mesh system is not set.");
		}
		return meshSystem->getMeshes();
	}

	std::unordered_map<std::string, std::shared_ptr<se::SEMaterial>>* getMaterials() const
	{
		if (materialSystem == nullptr)
		{
			throw std::runtime_error("Material system is not set.");
		}
		return materialSystem->getMaterials();
	}

	std::unordered_map<std::string, std::shared_ptr<se::SETexture>>* getTextures() const
	{
		if (textureSystem == nullptr)
		{
			throw std::runtime_error("Texture system is not set.");
		}
		return textureSystem->getTextures();
	}

	void setMaterialSystem(std::shared_ptr<se::MaterialSystem> materialSystem)  
	{  
		if (textureSystem == nullptr)
		{
			throw std::runtime_error("Texture system must be set before setting mesh system.");
		}
		materialSystem->setTextureSystem(textureSystem.get());
		this->materialSystem = materialSystem;  
	}  

	void setTextureSystem(std::shared_ptr<se::TextureSystem> textureSystem)  
	{  
		this->textureSystem = textureSystem;  
	}  

	void setMeshSystem(std::shared_ptr<se::MeshSystem> meshSystem)  
	{  
		if (materialSystem == nullptr)  
		{  
			throw std::runtime_error("Material system must be set before setting mesh system.");  
		}  

		if (textureSystem == nullptr)  
		{  
			throw std::runtime_error("Texture system must be set before setting mesh system.");  
		} 

		meshSystem->setMaterialSystem(this->materialSystem.get());
		meshSystem->setTextureSystem(this->textureSystem.get());
		this->meshSystem = meshSystem;  
	}  

	std::shared_ptr<se::SEMaterial> createMaterial(const std::string& name)  
	{  
		std::string guid = GenerateGUID();  
		auto material = materialSystem->CreatePBRMaterial(guid, name);  

		return material;  
	}  

	std::shared_ptr<se::SETexture> loadTexture(const std::string& path)  
	{  
		std::string guid = GenerateGUID();  
		auto texture = textureSystem->loadTexture(guid, getFileName(path), path);  

		return texture;  
	}

	std::shared_ptr<se::SEMesh> loadMesh(const std::string& path)
	{
		std::string guid = GenerateGUID();
		auto mesh = meshSystem->loadMesh(guid, getFileName(path), path);

		return mesh;
	}

	std::shared_ptr<se::SEMesh> createCube(const std::string& name)
	{
		std::string guid = GenerateGUID();
		auto mesh = meshSystem->createCube(guid, name);
		return mesh;
	}

	std::shared_ptr<se::SEMesh> createSphere(const std::string& name)
	{
		std::string guid = GenerateGUID();
		auto mesh = meshSystem->createSphere(guid, name);
		return mesh;
	}

private:  

	const std::string GenerateGUID()  
	{  
		static unsigned int idCounter = 0;  
		return "GUID_" + std::to_string(idCounter++);  
	}

	std::string getFileName(const std::string& path) {
		return std::filesystem::path(path).filename().string();
	}

	std::shared_ptr<se::MaterialSystem> materialSystem;  
	std::shared_ptr<se::TextureSystem> textureSystem;  
	std::shared_ptr<se::MeshSystem> meshSystem;  
};  
} // namespace se