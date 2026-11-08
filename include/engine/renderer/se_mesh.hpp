#pragma once

#include "engine/resources/se_resource.hpp" // Required base class: SEMesh derives from Resource.
#include <vulkan/vulkan.h> // Vulkan types appear in public method signatures.

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace se
{
    class SEDevice;
    class SEMaterial;
    class SESubMesh;
    struct SubMeshBuilder;
    struct SimplePushConstantData;

    class SEMesh : public Resource
    {
    public:
        SEMesh(SEDevice& device, const SubMeshBuilder& builder, const std::string& guid, const std::string& name);
        SEMesh(SEDevice& device, std::vector<std::unique_ptr<SESubMesh>> submeshes, const std::string& guid, const std::string& name);
        ~SEMesh();

        size_t getSubMeshCount() const;
        size_t getVerticesCount() const;
        size_t getIndicesCount() const;

        SEMesh(const SEMesh &) = delete;
        SEMesh &operator=(const SEMesh &) = delete;
        SEMesh(SEMesh&&) = delete;
        SEMesh& operator=(SEMesh&&) = delete;

        std::vector<std::unique_ptr<SESubMesh>> loadMesh(SEDevice &device, const std::string &path, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
        void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, std::shared_ptr<SEMaterial> goMaterial, SimplePushConstantData push, int frameIndex);

    private:
        SEDevice &seDevice;
        std::vector<std::unique_ptr<SESubMesh>> seSubmeshes;
    };
}
