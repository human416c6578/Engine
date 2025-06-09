#pragma once

#include "se_device.hpp"
#include "se_submesh.hpp"

// std
#include <vector>
#include <memory>

namespace se
{
    class SEMesh : public Resource
    {

    public:

        SEMesh(SEDevice& device, const SESubMesh::Builder builder, const std::string& guid, const std::string& name);
        SEMesh(SEDevice& device, std::vector<std::unique_ptr<SESubMesh>> submeshes, const std::string& guid, const std::string& name);
        ~SEMesh();

		/*
		std::shared_ptr<SEMaterial> getMaterial() const
		{
			return seMaterial;
		}

		void setMaterial(std::shared_ptr<SEMaterial> material)
		{
			seMaterial = material;
			for (auto& submesh : seSubmeshes)
			{
				submesh->setMaterial(seMaterial);
			}
		}
       
        bool hasMaterial() { return seMaterial != nullptr; }
        void bindMaterial(VkCommandBuffer commandBuffer) { seMaterial->bind(commandBuffer); }
		*/

		size_t getSubMeshCount() const
		{
			return seSubmeshes.size();
		}

        size_t getVerticesCount() const
        {
			size_t count = 0;
			for (const auto& submesh : seSubmeshes)
			{
				count += submesh->getVerticesCount();
			}
			return count;
        }

		size_t getIndicesCount() const
		{
			size_t count = 0;
			for (const auto& submesh : seSubmeshes)
			{
				count += submesh->getIndicesCount();
			}
			return count;
		}


        SEMesh(const SEMesh &) = delete;
        SEMesh &operator=(const SEMesh &) = delete;

        //void bind(VkCommandBuffer commandBuffer);
        std::vector<std::unique_ptr<SESubMesh>> loadMesh(SEDevice &device, const std::string &path, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);

		void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, std::shared_ptr<SEMaterial> goMaterial, SimplePushConstantData push, int frameIndex);

    private:
        SEDevice &seDevice;
        //std::shared_ptr<SEMaterial> seMaterial = nullptr;
        std::vector<std::unique_ptr<SESubMesh>> seSubmeshes;
    };

} // namespace se
