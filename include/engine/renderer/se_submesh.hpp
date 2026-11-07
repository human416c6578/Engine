#pragma once

#include "engine/vulkan/se_device.hpp"
#include "engine/core/se_vertex.hpp"
#include "engine/renderer/se_pbr_material.hpp"

// std
#include <vector>
#include <memory>

namespace se
{
    class SESubMesh
    {
    public:
        struct Builder
        {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
        };

        SESubMesh(SEDevice &device, const SESubMesh::Builder &builder);
        SESubMesh(SEDevice &device, const SESubMesh::Builder &builder, std::shared_ptr<SEMaterial> material);
        ~SESubMesh();

        bool hasMaterial() const { return seMaterial != nullptr; }
        void bindMaterial(VkCommandBuffer commandBuffer, int frameIndex) const { seMaterial->bind(commandBuffer, frameIndex); }

        void updateMaterial(int frameIndex) { seMaterial->update(frameIndex); }

        void setMaterial(std::shared_ptr<SEMaterial> material) { this->seMaterial = material; }

		size_t getVerticesCount() const
		{
			return vertexCount;
		}

        size_t getIndicesCount() const
        {
            return indexCount;
        }

        VkPipelineLayout getPipelineLayout() const
		{
			if (hasMaterial())
			{
				return seMaterial->getPipelineLayout();
			}
			return nullptr;
		}

        SESubMesh(const SESubMesh &) = delete;
        SESubMesh &operator=(const SESubMesh &) = delete;
        SESubMesh(SESubMesh&&) = delete;
        SESubMesh& operator=(SESubMesh&&) = delete;

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffers(const std::vector<Vertex> &vertices);
        void createIndexBuffers(const std::vector<uint32_t> &indices);
        void destroyOwnedHandles() noexcept;

        SEDevice &seDevice;
        std::shared_ptr<SEMaterial> seMaterial = nullptr;
        VkBuffer vertexBuffer{VK_NULL_HANDLE};
        VkDeviceMemory vertexBufferMemory{VK_NULL_HANDLE};
        uint32_t vertexCount{0};

        bool hasIndexBuffer{false};
        VkBuffer indexBuffer{VK_NULL_HANDLE};
        VkDeviceMemory indexBufferMemory{VK_NULL_HANDLE};
        uint32_t indexCount{0};
    };

} // namespace se
