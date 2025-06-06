#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "se_resource.hpp"

namespace se {

    class SEMaterialBase {
    public:
        enum class Type { PBR, Unlit, Custom };

        virtual ~SEMaterialBase() = default;

        // Bind the material's descriptor set(s) for rendering
        virtual void bind(VkCommandBuffer commandBuffer) = 0;

        virtual VkPipelineLayout getPipelineLayout() const = 0;

        virtual VkPipeline getPipeline() const = 0;

        // Get the descriptor set (for renderer use)
        virtual VkDescriptorSet getDescriptorSet() const = 0;

        // Get the descriptor set layout (for renderer or system use)
        virtual VkDescriptorSetLayout getDescriptorSetLayout() const = 0;

        // Get the material type (for runtime checks)
        virtual Type getType() const = 0;
    };

} // namespace se