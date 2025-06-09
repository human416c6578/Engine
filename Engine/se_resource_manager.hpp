#pragma once  

#include "se_pbr_material.hpp"  
#include "se_material_system.hpp"  
#include "se_texture_system.hpp"  
#include "se_mesh_system.hpp"

#include <random>
#include <sstream>
#include <iomanip>

namespace se  
{  
class ResourceManager  
{  
public:  
	ResourceManager();  
	~ResourceManager();  

	ResourceManager(const ResourceManager&) = delete;  
	ResourceManager& operator=(const ResourceManager&) = delete;

	std::shared_ptr<se::SEMesh> getMesh(const std::string& guid)
	{
		auto* meshes = getMeshes();
		auto it = meshes->find(guid);
		if (it != meshes->end()) {
			return it->second;
		}

		return nullptr;
	}

	std::shared_ptr<se::SEMaterial> getMaterial(const std::string& guid)
	{
		auto* materials = getMaterials();
		auto it = materials->find(guid);
		if (it != materials->end()) {
			return it->second;
		}

		return nullptr;
	}

	std::shared_ptr<se::SETexture> getTexture(const std::string& guid)
	{
		auto* textures = getTextures();
		auto it = textures->find(guid);
		if (it != textures->end()) {
			return it->second;
		}

		return nullptr;
	}

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

	std::shared_ptr<se::SEMaterial> createMaterial(const std::string& guid, const std::string& name)
	{
		auto material = materialSystem->CreatePBRMaterial(guid, name);

		return material;
	}

	std::shared_ptr<se::SETexture> loadTexture(const std::string& guid, const std::string& path)
	{
		auto texture = textureSystem->loadTexture(guid, getFileName(path), path);
		registerResourcePath(guid, path);

		return texture;
	}

	std::shared_ptr<se::SETexture> loadTexture(const std::string& path)  
	{  
		std::string guid = GenerateGUID();  
		auto texture = textureSystem->loadTexture(guid, getFileName(path), path);  
		registerResourcePath(guid, path);

		return texture;  
	}

	std::shared_ptr<se::SEMesh> loadMesh(const std::string& guid, const std::string& path)
	{
		auto mesh = meshSystem->loadMesh(guid, getFileName(path), path);
		registerResourcePath(guid, path);

		return mesh;
	}

	std::shared_ptr<se::SEMesh> loadMesh(const std::string& path)
	{
		std::string guid = GenerateGUID();
		auto mesh = meshSystem->loadMesh(guid, getFileName(path), path);
		registerResourcePath(guid, path);

		return mesh;
	}

	std::shared_ptr<se::SEMesh> createCube(const std::string& name)
	{
		auto mesh = getMesh("CUBE");
		if (!mesh)
		{
			mesh = meshSystem->createCube("CUBE", "CUBE");
			registerResourcePath("CUBE", "CUBE");
		}
		return mesh;
	}

	std::shared_ptr<se::SEMesh> createSphere(const std::string& name)
	{
		auto mesh = getMesh("SPHERE");
		if (!mesh)
		{
			mesh = meshSystem->createSphere("SPHERE", "SPHERE");
			registerResourcePath("SPHERE", "SPHERE");
		}
		return mesh;
	}

	void registerResourcePath(const std::string& guid, const std::string& path)
	{
		resourcePaths[guid] = path;
	}

	std::string getResourcePath(const std::string& guid) const
	{
		auto it = resourcePaths.find(guid);
		if (it != resourcePaths.end())
			return it->second;
		return {};
	}

private:  

	const std::string GenerateGUID()
	{
		static std::random_device rd;
		static std::mt19937_64 gen(rd());
		static std::uniform_int_distribution<uint64_t> dis;

		uint64_t part1 = dis(gen);
		uint64_t part2 = dis(gen);

		std::stringstream ss;
		ss << std::hex << std::setfill('0')
			<< std::setw(8) << ((part1 >> 32) & 0xFFFFFFFF) << "-"
			<< std::setw(4) << ((part1 >> 16) & 0xFFFF) << "-"
			<< std::setw(4) << ((part1) & 0xFFFF) << "-"
			<< std::setw(4) << ((part2 >> 48) & 0xFFFF) << "-"
			<< std::setw(12) << (part2 & 0xFFFFFFFFFFFF);

		return ss.str();
	}

	std::string getFileName(const std::string& path) {
		return std::filesystem::path(path).filename().string();
	}

	std::shared_ptr<se::MaterialSystem> materialSystem;  
	std::shared_ptr<se::TextureSystem> textureSystem;  
	std::shared_ptr<se::MeshSystem> meshSystem;

	std::unordered_map<std::string, std::string> resourcePaths;
};  
} // namespace se