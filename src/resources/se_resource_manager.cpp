#include "engine/resources/se_resource_manager.hpp"

#include "engine/renderer/se_material_system.hpp"
#include "engine/renderer/se_mesh.hpp"
#include "engine/renderer/se_mesh_system.hpp"
#include "engine/renderer/se_pbr_material.hpp"
#include "engine/renderer/se_texture.hpp"
#include "engine/renderer/se_texture_system.hpp"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <utility>

namespace se
{
    ResourceManager::ResourceManager(TextureSystem& textures, MaterialSystem& materials,
        MeshSystem& meshes)
        : textureSystem{textures}, materialSystem{materials}, meshSystem{meshes},
          guidGenerator{std::random_device{}()}
    {
    }

    std::shared_ptr<SEMesh> ResourceManager::getMesh(const std::string& guid)
    {
        const auto* meshes = getMeshes();
        const auto it = meshes->find(guid);
        return it != meshes->end() ? it->second : nullptr;
    }

    std::shared_ptr<SEMaterial> ResourceManager::getMaterial(const std::string& guid)
    {
        const auto* materials = getMaterials();
        const auto it = materials->find(guid);
        return it != materials->end() ? it->second : nullptr;
    }

    std::shared_ptr<SETexture> ResourceManager::getTexture(const std::string& guid)
    {
        const auto* textures = getTextures();
        const auto it = textures->find(guid);
        return it != textures->end() ? it->second : nullptr;
    }

    std::unordered_map<std::string, std::shared_ptr<SEMesh>>* ResourceManager::getMeshes() const
    {
        return meshSystem.getMeshes();
    }

    std::unordered_map<std::string, std::shared_ptr<SEMaterial>>* ResourceManager::getMaterials() const
    {
        return materialSystem.getMaterials();
    }

    std::unordered_map<std::string, std::shared_ptr<SETexture>>* ResourceManager::getTextures() const
    {
        return textureSystem.getTextures();
    }

    std::shared_ptr<SEMaterial> ResourceManager::createMaterial(const std::string& name)
    {
        return materialSystem.CreatePBRMaterial(GenerateGUID(), name);
    }

    std::shared_ptr<SEMaterial> ResourceManager::createMaterial(
        const std::string& guid, const std::string& name)
    {
        return materialSystem.CreatePBRMaterial(guid, name);
    }

    std::shared_ptr<SETexture> ResourceManager::loadTexture(
        const std::string& guid, const std::string& path)
    {
        auto texture = textureSystem.loadTexture(guid, getFileName(path), path);
        registerResourcePath(guid, path);
        return texture;
    }

    std::shared_ptr<SETexture> ResourceManager::loadTexture(const std::string& path)
    {
        const std::string guid = GenerateGUID();
        auto texture = textureSystem.loadTexture(guid, getFileName(path), path);
        registerResourcePath(guid, path);
        return texture;
    }

    std::shared_ptr<SEMesh> ResourceManager::loadMesh(
        const std::string& guid, const std::string& path)
    {
        auto mesh = meshSystem.loadMesh(guid, getFileName(path), path);
        registerResourcePath(guid, path);
        return mesh;
    }

    std::shared_ptr<SEMesh> ResourceManager::loadMesh(const std::string& path)
    {
        const std::string guid = GenerateGUID();
        auto mesh = meshSystem.loadMesh(guid, getFileName(path), path);
        registerResourcePath(guid, path);
        return mesh;
    }

    std::shared_ptr<SEMesh> ResourceManager::createPlane(const std::string& name)
    {
        auto mesh = getMesh("PLANE");
        if (!mesh)
        {
            mesh = meshSystem.createPlane("PLANE", "PLANE");
            registerResourcePath("PLANE", "PLANE");
        }
        return mesh;
    }

    std::shared_ptr<SEMesh> ResourceManager::createCube(const std::string& name)
    {
        auto mesh = getMesh("CUBE");
        if (!mesh)
        {
            mesh = meshSystem.createCube("CUBE", "CUBE");
            registerResourcePath("CUBE", "CUBE");
        }
        return mesh;
    }

    std::shared_ptr<SEMesh> ResourceManager::createSphere(const std::string& name)
    {
        auto mesh = getMesh("SPHERE");
        if (!mesh)
        {
            mesh = meshSystem.createSphere("SPHERE", "SPHERE");
            registerResourcePath("SPHERE", "SPHERE");
        }
        return mesh;
    }

    std::shared_ptr<SEMesh> ResourceManager::createCylinder(const std::string& name)
    {
        auto mesh = getMesh("CYLINDER");
        if (!mesh)
        {
            mesh = meshSystem.createCylinder("CYLINDER", name);
            registerResourcePath("CYLINDER", "CYLINDER");
        }
        return mesh;
    }

    void ResourceManager::registerResourcePath(const std::string& guid, const std::string& path)
    {
        resourcePaths[guid] = path;
    }

    std::string ResourceManager::getResourcePath(const std::string& guid) const
    {
        const auto it = resourcePaths.find(guid);
        return it != resourcePaths.end() ? it->second : std::string{};
    }

    std::string ResourceManager::GenerateGUID()
    {
        const uint64_t part1 = guidDistribution(guidGenerator);
        const uint64_t part2 = guidDistribution(guidGenerator);

        std::stringstream stream;
        stream << std::hex << std::setfill('0')
               << std::setw(8) << ((part1 >> 32) & 0xFFFFFFFF) << '-'
               << std::setw(4) << ((part1 >> 16) & 0xFFFF) << '-'
               << std::setw(4) << (part1 & 0xFFFF) << '-'
               << std::setw(4) << ((part2 >> 48) & 0xFFFF) << '-'
               << std::setw(12) << (part2 & 0xFFFFFFFFFFFF);
        return stream.str();
    }

    std::string ResourceManager::getFileName(const std::string& path)
    {
        return std::filesystem::path{path}.filename().string();
    }
}
