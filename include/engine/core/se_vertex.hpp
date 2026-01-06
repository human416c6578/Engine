#pragma once

#include "se_device.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace se
{
	struct Vertex
	{
		glm::vec3 position{};
		glm::vec3 normal{};
		glm::vec2 texCoord{};

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		

		bool operator==(const Vertex &other) const
		{
			return position == other.position && normal == other.normal && texCoord == other.texCoord;
		}
	};
}

namespace std {
    template<>
    struct hash<se::Vertex> {
        size_t operator()(const se::Vertex& vertex) const {
            size_t posHash = hash<glm::vec3>()(vertex.position);
            size_t normHash = hash<glm::vec3>()(vertex.normal);
            size_t texCoordHash = hash<glm::vec2>()(vertex.texCoord);

            // Combine the hashes using XOR and bit shifting
            return ((posHash ^ (normHash << 1)) >> 1) ^ (texCoordHash << 1);
        }
    };
}