#include "engine/renderer/se_submesh.hpp"
#include "engine/vulkan/se_device.hpp"
#include "engine/renderer/se_pbr_material.hpp"
#include "engine/vulkan/se_vk_result.hpp"

// std
#include <cassert>
#include <cstring>
#include <unordered_map>
#include <type_traits>
#include <utility>

namespace se
{
  namespace
  {
    struct ScopedStagingBuffer
    {
      explicit ScopedStagingBuffer(SEDevice& owner) : device{owner} {}
      ~ScopedStagingBuffer() noexcept { reset(); }

      ScopedStagingBuffer(const ScopedStagingBuffer&) = delete;
      ScopedStagingBuffer& operator=(const ScopedStagingBuffer&) = delete;
      ScopedStagingBuffer(ScopedStagingBuffer&& other) noexcept
          : device{other.device},
            buffer{std::exchange(other.buffer, VK_NULL_HANDLE)},
            memory{std::exchange(other.memory, VK_NULL_HANDLE)}
      {
      }
      ScopedStagingBuffer& operator=(ScopedStagingBuffer&&) = delete;

      void reset() noexcept
      {
        if (buffer != VK_NULL_HANDLE)
          vkDestroyBuffer(device.device(), std::exchange(buffer, VK_NULL_HANDLE), nullptr);
        if (memory != VK_NULL_HANDLE)
          vkFreeMemory(device.device(), std::exchange(memory, VK_NULL_HANDLE), nullptr);
      }

      SEDevice& device;
      VkBuffer buffer{VK_NULL_HANDLE};
      VkDeviceMemory memory{VK_NULL_HANDLE};
    };
    static_assert(!std::is_copy_constructible_v<ScopedStagingBuffer>);
    static_assert(std::is_nothrow_move_constructible_v<ScopedStagingBuffer>);
    static_assert(!std::is_move_assignable_v<ScopedStagingBuffer>);
  }
  SESubMesh::SESubMesh(SEDevice &device, const SESubMesh::Builder &builder) : seDevice{device}
  {
    try
    {
      createVertexBuffers(builder.vertices);
      createIndexBuffers(builder.indices);
    }
    catch (...)
    {
      destroyOwnedHandles();
      throw;
    }
  }

  SESubMesh::SESubMesh(SEDevice &device, const SESubMesh::Builder &builder, std::shared_ptr<SEMaterial> material) : seDevice{device}, seMaterial{material}
  {
    try
    {
      createVertexBuffers(builder.vertices);
      createIndexBuffers(builder.indices);
    }
    catch (...)
    {
      destroyOwnedHandles();
      throw;
    }
  }

  bool SESubMesh::hasMaterial() const { return seMaterial != nullptr; }

  void SESubMesh::bindMaterial(VkCommandBuffer commandBuffer, int frameIndex) const
  {
    seMaterial->bind(commandBuffer, frameIndex);
  }

  void SESubMesh::updateMaterial(int frameIndex) { seMaterial->update(frameIndex); }

  void SESubMesh::setMaterial(std::shared_ptr<SEMaterial> material) { seMaterial = std::move(material); }

  VkPipelineLayout SESubMesh::getPipelineLayout() const
  {
    return hasMaterial() ? seMaterial->getPipelineLayout() : VK_NULL_HANDLE;
  }

  SESubMesh::~SESubMesh()
  {
    destroyOwnedHandles();
  }

  void SESubMesh::destroyOwnedHandles() noexcept
  {
    if (indexBuffer != VK_NULL_HANDLE)
      vkDestroyBuffer(seDevice.device(), indexBuffer, nullptr);
    indexBuffer = VK_NULL_HANDLE;
    if (indexBufferMemory != VK_NULL_HANDLE)
      vkFreeMemory(seDevice.device(), indexBufferMemory, nullptr);
    indexBufferMemory = VK_NULL_HANDLE;

    if (vertexBuffer != VK_NULL_HANDLE)
      vkDestroyBuffer(seDevice.device(), vertexBuffer, nullptr);
    vertexBuffer = VK_NULL_HANDLE;
    if (vertexBufferMemory != VK_NULL_HANDLE)
      vkFreeMemory(seDevice.device(), vertexBufferMemory, nullptr);
    vertexBufferMemory = VK_NULL_HANDLE;
    hasIndexBuffer = false;
  }

  void SESubMesh::createVertexBuffers(const std::vector<Vertex> &vertices)
  {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    ScopedStagingBuffer staging{seDevice};
    seDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging.buffer, staging.memory);

    void* data = nullptr;
    checkVkResult(vkMapMemory(seDevice.device(), staging.memory, 0, bufferSize, 0, &data),
        "vkMapMemory(vertex staging buffer)");
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(seDevice.device(), staging.memory);

    seDevice.createBuffer(bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    seDevice.copyBuffer(staging.buffer, vertexBuffer, bufferSize);
  }

  void SESubMesh::createIndexBuffers(const std::vector<uint32_t> &indices)
  {
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;
    if (!hasIndexBuffer) return;

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
    ScopedStagingBuffer staging{seDevice};
    seDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging.buffer, staging.memory);

    void* data = nullptr;
    checkVkResult(vkMapMemory(seDevice.device(), staging.memory, 0, bufferSize, 0, &data),
        "vkMapMemory(index staging buffer)");
    std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(seDevice.device(), staging.memory);

    seDevice.createBuffer(bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    seDevice.copyBuffer(staging.buffer, indexBuffer, bufferSize);
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