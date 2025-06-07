#include "se_mesh.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace se
{
    SEMesh::SEMesh(SEDevice& device, const SESubMesh::Builder builder, const std::string& guid, const std::string& name) : seDevice{ device }, Resource(guid, name)
    {
        auto submesh = std::make_unique<SESubMesh>(device, builder);
        seSubmeshes.push_back(std::move(submesh));
    }

    SEMesh::SEMesh(SEDevice& device, std::vector<std::unique_ptr<SESubMesh>> submeshes, const std::string& guid, const std::string& name) : seDevice{ device }, Resource(guid, name)
    {
        this->seSubmeshes = std::move(submeshes);
    }

    SEMesh::~SEMesh()
    {
    }

    // Helper function to replace all backslashes with forward slashes
    static void replaceBackslashes(std::string& str)
    {
        std::replace(str.begin(), str.end(), '\\', '/');
    }

    static std::string GetBaseDir(const std::string& filepath)
    {
        if (filepath.find_last_of("/\\") != std::string::npos)
            return filepath.substr(0, filepath.find_last_of("/\\"));
        return "";
    }

    std::vector<std::unique_ptr<SESubMesh>> SEMesh::loadMesh(SEDevice& device, const std::string& path, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout)
    {
        std::vector<std::unique_ptr<SESubMesh>> submeshes;
        return submeshes;
    }

    /*
    std::vector<std::unique_ptr<SESubMesh>> SEMesh::loadMesh(SEDevice &device, const std::string &path, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout)
    {
        std::vector<std::unique_ptr<SESubMesh>> submeshes;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        std::string err;
        std::string warn;
        std::string base_dir = GetBaseDir(path);

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(),
                                    base_dir.c_str());

        if (!warn.empty())
        {
            std::cout << "WARN: " << warn << std::endl;
        }
        if (!err.empty())
        {
            std::cerr << err << std::endl;
        }

        // Check if normals are missing and need to be generated
        // bool regen_all_normals = attrib.normals.empty();
        if (true)
        {
            tinyobj::attrib_t outattrib;
            std::vector<tinyobj::shape_t> outshapes;
            computeSmoothingShapes(attrib, shapes, outshapes, outattrib);
            computeAllSmoothingNormals(outattrib, outshapes);
            shapes = outshapes;
            attrib = outattrib;
        }

        // Map material ID to a builder for that material
        std::unordered_map<int, SESubMesh::Builder> materialBuilders;

        // Process each shape in the loaded file
        for (const auto &shape : shapes)
        {
            int material_id = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0]; // Get the material ID
            auto &builder = materialBuilders[material_id];                                       // Get or create a builder for this material

            for (const auto &index : shape.mesh.indices)
            {
                Vertex vertex{};

                // Position
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

                // Normal
                if (index.normal_index >= 0)
                {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]};
                }
                else
                {
                    vertex.normal = {0.0f, 0.0f, 0.0f}; // Default normal
                }

                // Texture coordinate (Y-flip)
                if (index.texcoord_index >= 0)
                {
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
                }

                // Deduplicate vertices
                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(builder.vertices.size());
                    builder.vertices.push_back(vertex);
                }

                // Add the index of the vertex to the indices list
                builder.indices.push_back(uniqueVertices[vertex]);
            }
        }

        // Create submeshes for each material group
        for (auto &entry : materialBuilders)
        {
            int material_id = entry.first;
            SESubMesh::Builder &builder = entry.second;

            // Assign a material based on the material_id
            std::shared_ptr<SEMaterial> material;
            if (material_id >= 0 && material_id < materials.size())
            {
                const tinyobj::material_t &obj_material = materials[material_id];

                // Extract texture file paths from the material
                std::string diffuse_texture = obj_material.diffuse_texname;
                std::string normal_texture = obj_material.normal_texname;
                std::string specular_texture = obj_material.specular_texname;
                std::string ambient_texture = obj_material.ambient_texname;

                // Replace backslashes with forward slashes
                replaceBackslashes(diffuse_texture);
                replaceBackslashes(normal_texture);
                replaceBackslashes(specular_texture);
                replaceBackslashes(ambient_texture);

                // Create textures only if the file path is not empty
                std::optional<std::unique_ptr<se::SETexture>> diffTexture = std::nullopt;
                std::optional<std::unique_ptr<se::SETexture>> normTexture = std::nullopt;
                std::optional<std::unique_ptr<se::SETexture>> metallicTexture = std::nullopt;
                std::optional<std::unique_ptr<se::SETexture>> ambientTexture = std::nullopt;

                if (!diffuse_texture.empty())
                    diffTexture = std::make_unique<se::SETexture>(seDevice, base_dir + "/" + diffuse_texture);

                if (!normal_texture.empty())
                    normTexture = std::make_unique<se::SETexture>(seDevice, base_dir + "/" + normal_texture);

                if (!specular_texture.empty())
                    metallicTexture = std::make_unique<se::SETexture>(seDevice, base_dir + "/" + specular_texture);

                if (!ambient_texture.empty())
                    ambientTexture = std::make_unique<se::SETexture>(seDevice, base_dir + "/" + ambient_texture);

                // Convert shininess (Ns) to roughness
                float roughness = 1.0f - (obj_material.shininess / 1000.0f); // Inverse of shininess

                material = nullptr;
                // Initialize the SEMaterial with texture paths and material properties
                /*material = std::make_shared<se::SEMaterial>(
                    seDevice,
                    descriptorSetLayout,
                    VK_SAMPLE_COUNT_1_BIT,
                    obj_material.specular[0], roughness, 1.0f, // Default metallic, roughness, AO
                    std::move(diffTexture),                 // Diffuse texture
                    std::move(normTexture),                 // Normal texture
                    std::move(metallicTexture),             // Metallic texture (using specular as fallback)
                    std::nullopt,                           // Roughness texture (use scalar value instead)
                    std::move(ambientTexture)               // Ambient Occlusion texture
                );*/
                /*
                }

                else
                {
                    if(hasMaterial())
                        material = nullptr;
                    //else
                    //    material = std::make_shared<se::SEMaterial>(seDevice, descriptorSetLayout, VK_SAMPLE_COUNT_1_BIT, 1.0, 0.01, 1.0);
                }

                auto submesh = std::make_unique<SESubMesh>(device, builder, material);
                submeshes.push_back(std::move(submesh));
            }

            return submeshes;
        }
        */


    void SEMesh::draw(
        VkCommandBuffer commandBuffer,
        std::shared_ptr<SEMaterial> goMaterial,
        SimplePushConstantData push,
		int  frameIndex
        )
    {
        for (auto& submesh : seSubmeshes)
        {
            VkPipelineLayout pipelineLayout = nullptr;

            if (goMaterial)
            {
				goMaterial->update(frameIndex);
                goMaterial->bind(commandBuffer, frameIndex);
                pipelineLayout = goMaterial->getPipelineLayout();
            }
            else if (submesh->hasMaterial())
            {
                submesh->updateMaterial(frameIndex);
                submesh->bindMaterial(commandBuffer, frameIndex);
                pipelineLayout = submesh->getPipelineLayout();
            }

            if (pipelineLayout == nullptr)
            {
                // Skip submesh if no valid pipeline layout
                continue;
            }

            submesh->bind(commandBuffer);

            vkCmdPushConstants(
                commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(SimplePushConstantData),
                &push);

            submesh->draw(commandBuffer);
        }
    }


}