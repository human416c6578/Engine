#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace se
{
    using EmitterHandle = uint32_t;
    using ForceHandle = uint32_t;
    static constexpr uint32_t INVALID_HANDLE = ~0u;

    class ParticleSystem;

    struct ParticleEmitterComponent
    {
        EmitterHandle handle = INVALID_HANDLE;
        ParticleSystem* system = nullptr;

        bool isValid() const noexcept { return handle != INVALID_HANDLE && system != nullptr; }

        void syncTransform(const glm::vec3& worldPos);
        void setActive(bool active);
        void release(); // Remove the borrowed system's emitter before scene deletion.

        void syncTransform(const glm::vec3& worldPos, const glm::vec3& offset)
        {
            syncTransform(worldPos + offset);
        }
    };
}
