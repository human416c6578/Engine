#pragma once

#include "se_device.hpp"
#include "se_vertex.hpp"
#include "se_material.hpp"

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
        void bindMaterial(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const { seMaterial->bind(commandBuffer, pipelineLayout); }

        SESubMesh(const SESubMesh &) = delete;
        SESubMesh &operator=(const SESubMesh &) = delete;

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffers(const std::vector<Vertex> &vertices);
        void createIndexBuffers(const std::vector<uint32_t> &indices);

        SEDevice &seDevice;
        std::shared_ptr<SEMaterial> seMaterial = nullptr;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        uint32_t vertexCount;

        bool hasIndexBuffer = false;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        uint32_t indexCount;
    };

} // namespace se
