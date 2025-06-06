#pragma once  

#include <string>  
#include <vector>  
#include <unordered_map>  
#include <memory>  

#include "se_device.hpp"
#include "se_material_system.hpp"
#include "se_texture_system.hpp"
#include "se_mesh.hpp"

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure)

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>




namespace se
{
	class MeshSystem
	{
	public:
		MeshSystem(se::SEDevice& device);
		~MeshSystem();

		MeshSystem(const MeshSystem&) = delete;
		MeshSystem& operator=(const MeshSystem&) = delete;

		std::unordered_map<std::string, std::shared_ptr<se::SEMesh>>* getMeshes()
		{
			return &meshes;
		}


		void setMaterialSystem(se::MaterialSystem* materialSystem)
		{
			this->materialSystem = materialSystem;
		}

		void setTextureSystem(se::TextureSystem* textureSystem)
		{
			this->textureSystem = textureSystem;
		}

		std::shared_ptr<se::SEMesh> loadMesh(const std::string& guid, const std::string& name, const std::string& path);

		std::shared_ptr<se::SEMesh> createCube(const std::string& guid, const std::string& name);
		std::shared_ptr<se::SEMesh> createSphere(const std::string& guid, const std::string& name);

	private:

		se::SEDevice& seDevice;
		se::MaterialSystem* materialSystem = nullptr;
		se::TextureSystem* textureSystem = nullptr;

		std::unordered_map<std::string, std::shared_ptr<se::SEMesh>> meshes;

		std::string GetBaseDir(const std::string& path);
	};
} // namespace se