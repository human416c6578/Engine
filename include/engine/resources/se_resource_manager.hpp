#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>

namespace se
{
    class MaterialSystem;
    class MeshSystem;
    class TextureSystem;
    class SEMaterial;
    class SEMesh;
    class SETexture;

    class ResourceManager
    {
    public:
        ResourceManager(TextureSystem& textures, MaterialSystem& materials, MeshSystem& meshes);
        ~ResourceManager() = default;

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;
        ResourceManager(ResourceManager&&) = delete;
        ResourceManager& operator=(ResourceManager&&) = delete;

        std::shared_ptr<SEMesh> getMesh(const std::string& guid);
        std::shared_ptr<SEMaterial> getMaterial(const std::string& guid);
        std::shared_ptr<SETexture> getTexture(const std::string& guid);

        std::unordered_map<std::string, std::shared_ptr<SEMesh>>* getMeshes() const;
        std::unordered_map<std::string, std::shared_ptr<SEMaterial>>* getMaterials() const;
        std::unordered_map<std::string, std::shared_ptr<SETexture>>* getTextures() const;

        std::shared_ptr<SEMaterial> createMaterial(const std::string& name);
        std::shared_ptr<SEMaterial> createMaterial(const std::string& guid, const std::string& name);
        std::shared_ptr<SETexture> loadTexture(const std::string& guid, const std::string& path);
        std::shared_ptr<SETexture> loadTexture(const std::string& path);
        std::shared_ptr<SEMesh> loadMesh(const std::string& guid, const std::string& path);
        std::shared_ptr<SEMesh> loadMesh(const std::string& path);

        std::shared_ptr<SEMesh> createPlane(const std::string& name);
        std::shared_ptr<SEMesh> createCube(const std::string& name);
        std::shared_ptr<SEMesh> createSphere(const std::string& name);

        void registerResourcePath(const std::string& guid, const std::string& path);
        std::string getResourcePath(const std::string& guid) const;

    private:
        std::string GenerateGUID();
        static std::string getFileName(const std::string& path);

        TextureSystem& textureSystem;
        MaterialSystem& materialSystem;
        MeshSystem& meshSystem;
        std::mt19937_64 guidGenerator;
        std::uniform_int_distribution<uint64_t> guidDistribution{};
        std::unordered_map<std::string, std::string> resourcePaths;
    };
}
