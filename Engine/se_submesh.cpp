#include "se_submesh.hpp"

// std
#include <cassert>
#include <cstring>
#include <unordered_map>

namespace se
{
  SESubMesh::SESubMesh(SEDevice &device, const SESubMesh::Builder &builder) : seDevice{device}
  {
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
  }

  SESubMesh::SESubMesh(SEDevice &device, const SESubMesh::Builder &builder, std::shared_ptr<SEMaterial> material) : seDevice{device}, seMaterial{material}
  {
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
  }

  SESubMesh::~SESubMesh()
  {
    vkDestroyBuffer(seDevice.device(), vertexBuffer, nullptr);
    vkFreeMemory(seDevice.device(), vertexBufferMemory, nullptr);

    if (hasIndexBuffer)
    {
      vkDestroyBuffer(seDevice.device(), indexBuffer, nullptr);
      vkFreeMemory(seDevice.device(), indexBufferMemory, nullptr);
    }
  }

  void SESubMesh::createVertexBuffers(const std::vector<Vertex> &vertices)
  {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    seDevice.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void *data;
    vkMapMemory(seDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(seDevice.device(), stagingBufferMemory);

    seDevice.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer,
        vertexBufferMemory);

    seDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);
  }

  void SESubMesh::createIndexBuffers(const std::vector<uint32_t> &indices)
  {
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;

    if (!hasIndexBuffer)
    {
      return;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    seDevice.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void *data;
    vkMapMemory(seDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(seDevice.device(), stagingBufferMemory);

    seDevice.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer,
        indexBufferMemory);

    seDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(seDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(seDevice.device(), stagingBufferMemory, nullptr);
  }


  void SESubMesh::draw(VkCommandBuffer commandBuffer)
  {
    if (hasIndexBuffer)
    {
      vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    }
    else
    {
      vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
  }

  void SESubMesh::bind(VkCommandBuffer commandBuffer)
  {
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (hasIndexBuffer)
    {
      vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
  }

}