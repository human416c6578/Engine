#include "engine/renderer/se_mesh.hpp"

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

    void SEMesh::draw(
        VkCommandBuffer commandBuffer,
        VkPipelineLayout pipelineLayout,
        std::shared_ptr<SEMaterial> goMaterial,
        SimplePushConstantData push,
		int  frameIndex
        )
    {
        if (goMaterial)
        {
            goMaterial->update(frameIndex);
            goMaterial->bind(commandBuffer, frameIndex);
        }

        for (auto& submesh : seSubmeshes)
        {
            if (submesh->hasMaterial())
            {
                submesh->updateMaterial(frameIndex);
                submesh->bindMaterial(commandBuffer, frameIndex);
            }

            if (!goMaterial && !submesh->hasMaterial())
                continue;

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