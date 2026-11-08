#include "engine/renderer/se_mesh_system.hpp"
#include "engine/renderer/se_pbr_material.hpp"
#include "engine/renderer/se_material_system.hpp"
#include "engine/renderer/se_mesh.hpp"
#include "engine/renderer/se_submesh.hpp"
#include "engine/renderer/se_texture_system.hpp"
#include "engine/vulkan/se_device.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>

#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure)

namespace se
{
	MeshSystem::MeshSystem(se::SEDevice& device, se::MaterialSystem& materials,
		se::TextureSystem& textures)
		: seDevice{device}, materialSystem{materials}, textureSystem{textures}
	{
	}

	MeshSystem::~MeshSystem()
	{
	}

	std::shared_ptr<se::SEMesh> MeshSystem::loadMesh(const std::string& guid, const std::string& name, const std::string& path)
	{
        Assimp::Importer Importer;
        const aiScene* pScene = Importer.ReadFile(path, ASSIMP_LOAD_FLAGS);

		if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
		{
			seDevice.log(LogLevel::Error, "Assimp", "%s", Importer.GetErrorString());
			return nullptr;
		}

        std::vector<std::unique_ptr<SESubMesh>> submeshes;
        std::vector<std::shared_ptr<SEMaterial>> materials;

		if (pScene->HasMeshes())
		{
            if (pScene->HasMaterials())
            {

				for (unsigned int i = 0; i < pScene->mNumMaterials; ++i)
				{
					aiMaterial* material = pScene->mMaterials[i];

					std::string name = material->GetName().C_Str();
                    std::string base_path = GetBaseDir(path) + "/";

                    std::shared_ptr<se::SEMaterial> seMaterial = materialSystem.CreatePBRMaterial(guid + "_material_" + std::to_string(i), name + "_material_" + std::to_string(i));

                    float metallic = 0.05f;
                    float roughness = 0.95f;

                    material->Get(AI_MATKEY_REFLECTIVITY, metallic);
					metallic = glm::clamp(metallic, 0.05f, 0.95f);

                    float shininess = 0.0f;
                    if (material->Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS) {
                        roughness = 1.0f - glm::clamp(shininess / 1000.0f, 0.05f, 0.95f);
                    }

                    seMaterial->setMetallic(metallic);
                    seMaterial->setRoughness(roughness);
                    seMaterial->setAO(1.0f);

                    aiString diffuseTexturePath, normalTexturePath, specularTexturePath, roughnessTexturePath, aoTexturePath;
                    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == aiReturn_SUCCESS)
                    {
                        std::shared_ptr<se::SETexture> diffuseTexture = textureSystem.loadTexture(guid + "_texture_" + std::to_string(i), name + "_texture_" + std::to_string(i), base_path + diffuseTexturePath.C_Str());
                        seMaterial->setDiffuseTexture(diffuseTexture);
                    }
                    if (material->GetTexture(aiTextureType_NORMALS, 0, &normalTexturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == aiReturn_SUCCESS)
                    {
                        std::shared_ptr<se::SETexture> normalTexture = textureSystem.loadTexture(guid + "_texture_" + std::to_string(i), name + "_texture_" + std::to_string(i), base_path + normalTexturePath.C_Str());
                        seMaterial->setNormalTexture(normalTexture);
                    }

                    if (material->GetTexture(aiTextureType_METALNESS, 0, &specularTexturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == aiReturn_SUCCESS)
                    {
                        std::shared_ptr<se::SETexture> specularTexture = textureSystem.loadTexture(guid + "_texture_" + std::to_string(i), name + "_texture_" + std::to_string(i), base_path + specularTexturePath.C_Str());
						seMaterial->setMetallicTexture(specularTexture);
                    }
                    else if(material->GetTexture(aiTextureType_SPECULAR, 0, &specularTexturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == aiReturn_SUCCESS)
                    {
                        std::shared_ptr<se::SETexture> specularTexture = textureSystem.loadTexture(guid + "_texture_" + std::to_string(i), name + "_texture_" + std::to_string(i), base_path + specularTexturePath.C_Str());
                        seMaterial->setMetallicTexture(specularTexture);
                    }
                    if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughnessTexturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == aiReturn_SUCCESS)
                    {
                        std::shared_ptr<se::SETexture> roughnessTexture = textureSystem.loadTexture(guid + "_texture_" + std::to_string(i), name + "_texture_" + std::to_string(i), base_path + roughnessTexturePath.C_Str());
                        seMaterial->setRoughnessTexture(roughnessTexture);
                    }
                    else if (material->GetTexture(aiTextureType_SHININESS, 0, &roughnessTexturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == aiReturn_SUCCESS)
                    {
                        std::shared_ptr<se::SETexture> roughnessTexture = textureSystem.loadTexture(guid + "_texture_" + std::to_string(i), name + "_texture_" + std::to_string(i), base_path + roughnessTexturePath.C_Str());
						seMaterial->setRoughnessTexture(roughnessTexture);
                    }

                    if (material->GetTexture(aiTextureType_AMBIENT, 0, &aoTexturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == aiReturn_SUCCESS)
                    {
                        std::shared_ptr<se::SETexture> aoTexture = textureSystem.loadTexture(guid + "_texture_" + std::to_string(i), name + "_texture_" + std::to_string(i), base_path + aoTexturePath.C_Str());
						seMaterial->setAOTexture(aoTexture);
                    }


					materials.push_back(seMaterial);
				}
            }

			for (unsigned int i = 0; i < pScene->mNumMeshes; ++i)
			{
                se::SESubMesh::Builder builder{};
				const aiMesh* mesh = pScene->mMeshes[i];

				for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
				{
					glm::vec3 position(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);
					glm::vec3 normal(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
					glm::vec2 texCoord(0.0f, 0.0f);
					if (mesh->HasTextureCoords(0))
					{
						texCoord.x = mesh->mTextureCoords[0][j].x;
						texCoord.y = mesh->mTextureCoords[0][j].y;
					}

					builder.vertices.push_back({ position, normal, texCoord});
				}
				for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
				{
					const aiFace& face = mesh->mFaces[j];
					for (unsigned int k = 0; k < face.mNumIndices; ++k)
					{
						builder.indices.push_back(face.mIndices[k]);
					}
				}

                std::unique_ptr<se::SESubMesh> subMesh = std::make_unique<se::SESubMesh>(seDevice, builder);
                subMesh->setMaterial(materials.at(mesh->mMaterialIndex));
				submeshes.push_back(std::move(subMesh));
			}
			auto newMesh = std::make_shared<se::SEMesh>(seDevice, std::move(submeshes), guid, name);
			meshes[guid] = newMesh;
			return newMesh;
		}

		return std::shared_ptr<se::SEMesh>();
	}

	std::shared_ptr<se::SEMesh> MeshSystem::createPlane(const std::string& guid, const std::string& name)
    {
        se::SESubMesh::Builder builder{};

        // Vertex positions
        glm::vec3 pos1(-0.5f, 0.0f,  0.5f);
        glm::vec3 pos2(-0.5f, 0.0f, -0.5f);
        glm::vec3 pos3( 0.5f, 0.0f, -0.5f);
        glm::vec3 pos4( 0.5f, 0.0f,  0.5f);

        // Texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);

        // Normal (up for plane)
        glm::vec3 normal(0.0f, -1.0f, 0.0f);

        // Compute tangent and bitangent
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent = glm::normalize(tangent);

        glm::vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent = glm::normalize(bitangent);

        // Create vertices
        builder.vertices = {
            {pos1, normal, uv1},
            {pos2, normal, uv2},
            {pos3, normal, uv3},
            {pos4, normal, uv4}
        };

        // Indices for two triangles
        builder.indices = {
            0, 1, 2,
            0, 2, 3
        };

        auto planeMesh = std::make_shared<se::SEMesh>(seDevice, builder, guid, name);
        meshes[guid] = planeMesh;

        return planeMesh;
    }


    std::shared_ptr<se::SEMesh> MeshSystem::createCube(const std::string& guid, const std::string& name)
    {
        se::SESubMesh::Builder builder{};

        auto computeBitangent = [](const glm::vec3& N, const glm::vec3& T) {
            return glm::normalize(glm::cross(N, T));
        };

        builder.vertices = {
            // LEFT (-X)
            {{-0.5f, -0.5f,  0.5f}, {-1,0,0}, {0,0}},
            {{-0.5f,  0.5f,  0.5f}, {-1,0,0}, {1,0}},
            {{-0.5f, -0.5f, -0.5f}, {-1,0,0}, {0,1}},
            {{-0.5f,  0.5f, -0.5f}, {-1,0,0}, {1,1}},

            // RIGHT (+X)
            {{ 0.5f, -0.5f, -0.5f}, {1,0,0}, {0,0}},
            {{ 0.5f,  0.5f, -0.5f}, {1,0,0}, {1,0}},
            {{ 0.5f, -0.5f,  0.5f}, {1,0,0}, {0,1}},
            {{ 0.5f,  0.5f,  0.5f}, {1,0,0}, {1,1}},

            // TOP (+Y)
            {{-0.5f, 0.5f, -0.5f}, {0,1,0}, {0,0}},
            {{ 0.5f, 0.5f, -0.5f}, {0,1,0}, {1,0}},
            {{-0.5f, 0.5f,  0.5f}, {0,1,0}, {0,1}},
            {{ 0.5f, 0.5f,  0.5f}, {0,1,0}, {1,1}},

            // BOTTOM (-Y)
            {{-0.5f,-0.5f,  0.5f}, {0,-1,0}, {0,0}},
            {{ 0.5f,-0.5f,  0.5f}, {0,-1,0}, {1,0}},
            {{-0.5f,-0.5f, -0.5f}, {0,-1,0}, {0,1}},
            {{ 0.5f,-0.5f, -0.5f}, {0,-1,0}, {1,1}},

            // FRONT (+Z)
            {{-0.5f,-0.5f, 0.5f}, {0,0,1}, {0,0}},
            {{ 0.5f,-0.5f, 0.5f}, {0,0,1}, {1,0}},
            {{-0.5f, 0.5f, 0.5f}, {0,0,1}, {0,1}},
            {{ 0.5f, 0.5f, 0.5f}, {0,0,1}, {1,1}},

            // BACK (-Z)
            {{ 0.5f,-0.5f,-0.5f}, {0,0,-1}, {0,0}},
            {{-0.5f,-0.5f,-0.5f}, {0,0,-1}, {1,0}},
            {{ 0.5f, 0.5f,-0.5f}, {0,0,-1}, {0,1}},
            {{-0.5f, 0.5f,-0.5f}, {0,0,-1}, {1,1}}
        };

        builder.indices = {
            0,1,2, 1,3,2,       // LEFT
            4,5,6, 5,7,6,       // RIGHT
            8,9,10, 9,11,10,    // TOP
            12,13,14, 13,15,14, // BOTTOM
            16,17,18, 17,19,18, // FRONT
            20,21,22, 21,23,22  // BACK
        };

        auto cubeMesh = std::make_shared<se::SEMesh>(seDevice, builder, guid, name);
        meshes[guid] = cubeMesh;

        return cubeMesh;
    }


    std::shared_ptr<se::SEMesh> MeshSystem::createSphere(const std::string& guid, const std::string& name)
    {
        se::SESubMesh::Builder builder{};
        float radius = 1.0f;
        int sectorCount = 20;
        int stackCount = 20;


        // Variables for spherical coordinates
        float sectorStep = 2 * glm::pi<float>() / sectorCount;  // Angle between each sector
        float stackStep = glm::pi<float>() / stackCount;        // Angle between each stack

        // Generate vertices
        for (int i = 0; i <= stackCount; ++i)
        {
            float stackAngle = glm::pi<float>() / 2 - i * stackStep; // Angle from the top (-π/2) to the bottom (π/2)
            float xy = radius * cos(stackAngle);                     // Radius at this stack level
            float z = radius * sin(stackAngle);                      // Z position at this stack level

            for (int j = 0; j <= sectorCount; ++j)
            {
                float sectorAngle = j * sectorStep; // Angle around the Y-axis (0 to 2π)

                // Calculate vertex position in Cartesian coordinates
                glm::vec3 position = glm::vec3(
                    xy * cos(sectorAngle), // X position
                    xy * sin(sectorAngle), // Y position
                    z                      // Z position
                );

                // Add the vertex with position and normal (normalized)
                builder.vertices.push_back({
                    position,
                    glm::normalize(position),
                    glm::vec2((float)j / sectorCount, (float)i / stackCount)
                    });
            }
        }

        // Generate indices
        for (int i = 0; i < stackCount; ++i)
        {
            int k1 = i * (sectorCount + 1);     // First vertex of the current stack
            int k2 = k1 + sectorCount + 1;      // First vertex of the next stack

            for (int j = 0; j < sectorCount; ++j)
            {
                if (i != 0) // Skip the first stack (the top of the sphere)
                {
                    builder.indices.push_back(k1 + j);
                    builder.indices.push_back(k2 + j);
                    builder.indices.push_back(k1 + j + 1);
                }

                if (i != (stackCount - 1)) // Skip the last stack (the bottom of the sphere)
                {
                    builder.indices.push_back(k1 + j + 1);
                    builder.indices.push_back(k2 + j);
                    builder.indices.push_back(k2 + j + 1);
                }
            }
        }

        auto sphereMesh = std::make_shared<se::SEMesh>(seDevice, builder, guid, name);

        meshes[guid] = sphereMesh;

        return sphereMesh;
    }

    std::string MeshSystem::GetBaseDir(const std::string& path)
    {
        return std::filesystem::path(path).parent_path().string();
    }
}
