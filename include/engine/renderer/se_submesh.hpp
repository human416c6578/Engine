#pragma once

#include "engine/renderer/se_vertex.hpp" // Builder stores Vertex values.
#include <vulkan/vulkan.h> // Vulkan handles and command types are part of this interface.

#include <cstdint>
#include <memory>
#include <vector>

namespace se
{
    class SEDevice;
    class SEMaterial;
    struct DeferredDisposal;

    struct SubMeshBuilder
    {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
    };

    class SESubMesh
    {
    public:
        using Builder = SubMeshBuilder;

        SESubMesh(SEDevice &device, const Builder &builder);
        SESubMesh(SEDevice &device, const Builder &builder, std::shared_ptr<SEMaterial> material);
        ~SESubMesh();

        bool hasMaterial() const;
        void bindMaterial(VkCommandBuffer commandBuffer, int frameIndex) const;
        void updateMaterial(int frameIndex);
        void setMaterial(std::shared_ptr<SEMaterial> material);

        size_t getVerticesCount() const { return vertexCount; }
        size_t getIndicesCount() const { return indexCount; }
        VkPipelineLayout getPipelineLayout() const;

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
        // Reserved before Vulkan handles; transfer to the device without allocating on release.
        std::unique_ptr<DeferredDisposal> disposalRecord;
        std::shared_ptr<SEMaterial> seMaterial{};
        VkBuffer vertexBuffer{VK_NULL_HANDLE};
        VkDeviceMemory vertexBufferMemory{VK_NULL_HANDLE};
        uint32_t vertexCount{0};
        bool hasIndexBuffer{false};
        VkBuffer indexBuffer{VK_NULL_HANDLE};
        VkDeviceMemory indexBufferMemory{VK_NULL_HANDLE};
        uint32_t indexCount{0};
    };
}
