#pragma once

#include "se_device.hpp"
#include "se_submesh.hpp"

// std
#include <vector>
#include <memory>

namespace se
{
    class SEMesh
    {

    public:

        SEMesh(SEDevice &device, const std::string &path, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
        SEMesh(SEDevice &device, const std::string &path, std::shared_ptr<SEMaterial> material, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
        SEMesh(SEDevice &device, const SESubMesh::Builder builder);
        SEMesh(SEDevice &device, const SESubMesh::Builder builder, std::shared_ptr<SEMaterial> material);
        ~SEMesh();

        

        bool hasMaterial() { return seMaterial != nullptr; }
        void bindMaterial(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) { seMaterial->bind(commandBuffer, pipelineLayout); }

        SEMesh(const SEMesh &) = delete;
        SEMesh &operator=(const SEMesh &) = delete;

        //void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer, SimplePushConstantData push, VkPipelineLayout pipelineLayout);

        std::vector<std::unique_ptr<SESubMesh>> loadMesh(SEDevice &device, const std::string &path, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);

    private:
        SEDevice &seDevice;
        std::shared_ptr<SEMaterial> seMaterial = nullptr;
        std::vector<std::unique_ptr<SESubMesh>> seSubmeshes;
    };

} // namespace se
