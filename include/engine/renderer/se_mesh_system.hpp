#pragma once

#include <string>
#include <unordered_map>
#include <memory>


namespace se
{
	class SEDevice;
	class MaterialSystem;
	class TextureSystem;
	class SEMesh;

	class MeshSystem
	{
	public:
		MeshSystem(se::SEDevice& device, se::MaterialSystem& materials,
			se::TextureSystem& textures);
		~MeshSystem();

		MeshSystem(const MeshSystem&) = delete;
		MeshSystem& operator=(const MeshSystem&) = delete;
		MeshSystem(MeshSystem&&) = delete;
		MeshSystem& operator=(MeshSystem&&) = delete;

		std::unordered_map<std::string, std::shared_ptr<se::SEMesh>>* getMeshes()
		{
			return &meshes;
		}


		std::shared_ptr<se::SEMesh> loadMesh(const std::string& guid, const std::string& name, const std::string& path);

		std::shared_ptr<se::SEMesh> createPlane(const std::string& guid, const std::string& name);
		std::shared_ptr<se::SEMesh> createCube(const std::string& guid, const std::string& name);
		std::shared_ptr<se::SEMesh> createSphere(const std::string& guid, const std::string& name);

	private:

		se::SEDevice& seDevice;
		se::MaterialSystem& materialSystem;
		se::TextureSystem& textureSystem;

		std::unordered_map<std::string, std::shared_ptr<se::SEMesh>> meshes;

		std::string GetBaseDir(const std::string& path);
	};
} // namespace se
