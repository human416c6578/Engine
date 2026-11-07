#pragma once
#include "engine/vulkan/se_device.hpp"
#include "engine/renderer/se_pipeline.hpp"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace se
{
    static constexpr uint32_t MAX_PARTICLES = 1000000;
    static constexpr uint32_t MAX_EMITTERS  = 64;
    static constexpr uint32_t MAX_FORCES    = 32;

    enum class VolumeType : uint32_t
    {
        Point  = 0,  // single point
        Sphere = 1,  // random point inside sphere
        Box    = 2,  // random point inside AABB
        Cone   = 3,  // random point inside cone
        Ring   = 4,  // random point on a ring/torus
    };

    struct SpawnVolume
    {
        glm::vec4   position;   // xyz = center, w = unused
        glm::vec4   extents;    // sphere: x=radius  box: xyz=half-extents
                                // cone:   x=radius y=height  ring: x=innerR y=outerR
        glm::vec4   direction;  // xyz = up/forward axis for cone/ring, w = unused
        VolumeType  type       = VolumeType::Point;
        float       padding[3] = {};
    };

    enum class ForceType : uint32_t
    {
        Directional = 0,  // constant force (gravity, wind)
        Point       = 1,  // attract/repel from a point (black hole, magnet)
        Vortex      = 2,  // spin particles around an axis
        Turbulence  = 3,  // noise-based random force
        Drag        = 4,  // velocity damping
    };

    struct Force
    {
        glm::vec4  position;   // world pos (for Point/Vortex)
        glm::vec4  direction;  // force direction or axis (for Directional/Vortex)
        float      strength;   // magnitude
        float      radius;     // influence radius (0 = infinite)
        float      falloff;    // 0=constant 1=linear 2=quadratic
        ForceType  type;
        uint32_t   active = 1;
        float      padding[3] = {};
    };

    struct Emitter
    {
        // Identity
        uint32_t  id          = 0;
        uint32_t  active      = 1;
        float     padding[2]  = {};

        SpawnVolume volume{};

        glm::vec4 direction;    // xyz = base emit direction, w = spread (radians)
        float     speedMin     = 1.f;
        float     speedMax     = 3.f;

        float     lifetimeMin  = 1.f;
        float     lifetimeMax  = 3.f;

        glm::vec4 colorStart   = { 1.f, 1.f, 1.f, 1.f };
        glm::vec4 colorEnd     = { 1.f, 1.f, 1.f, 0.f };
        float     sizeStart    = 4.f;
        float     sizeEnd      = 1.f;
        float     emitRate     = 100.f;

        uint32_t  forceIndices[8] = { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u };
        uint32_t  forceCount      = 0;
        float     padding2[4]     = {};
    };

    struct Particle
    {
        glm::vec4 position;    // xyz = pos,        w = lifetime remaining
        glm::vec4 velocity;    // xyz = vel,         w = total lifetime
        glm::vec4 colorStart;
        glm::vec4 colorEnd;
        float     sizeStart;
        float     sizeEnd;
        uint32_t  emitterID;   // which emitter owns this particle
        float     padding;
    };

    struct ParticleFrameUBO
    {
        float    deltaTime;
        float    totalTime;
        uint32_t particleCount;
        uint32_t emitterCount;
        uint32_t forceCount;
        float    padding[3];
    };

    using EmitterHandle = uint32_t;
    using ForceHandle   = uint32_t;
    static constexpr uint32_t INVALID_HANDLE = ~0u;

    struct ParticleBuffer
    {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void*          mapped = nullptr;
    };

    class ParticleSystem
    {
    public:
        ParticleSystem(SEDevice& device, VkRenderPass renderPass);
        ~ParticleSystem();

        ParticleSystem(const ParticleSystem&)            = delete;
        ParticleSystem& operator=(const ParticleSystem&) = delete;

        void update(VkCommandBuffer commandBuffer, int frameIndex,
                    float deltaTime, float totalTime);
        void render(VkCommandBuffer commandBuffer, int frameIndex,
                    glm::mat4 projView);

        EmitterHandle addEmitter(const Emitter& emitter);
        void          removeEmitter(EmitterHandle handle);
        Emitter&      getEmitter(EmitterHandle handle);
        void          setEmitterActive(EmitterHandle handle, bool active);

        ForceHandle   addForce(const Force& force);
        void          removeForce(ForceHandle handle);
        Force&        getForce(ForceHandle handle);

        void          attachForce(EmitterHandle emitter, ForceHandle force);
        void          detachForce(EmitterHandle emitter, ForceHandle force);

        struct FireSetup   { EmitterHandle emitter; ForceHandle upDraft; ForceHandle turbulence;};
        struct RainSetup   { EmitterHandle emitter; ForceHandle gravity; ForceHandle wind; };
        struct VortexSetup { EmitterHandle emitter; ForceHandle vortex;  ForceHandle drag; ForceHandle lift; };
        struct BlackholeSetup { EmitterHandle emitter; ForceHandle gravity; ForceHandle turbulence; ForceHandle vortex; ForceHandle drag; ForceHandle lift; };

        FireSetup   makeFire(glm::vec3 position, float scale = 1.f);
        RainSetup   makeRain(glm::vec3 position, glm::vec3 area);
        RainSetup   makeSnow(glm::vec3 position, glm::vec3 area);
        VortexSetup makeVortex(glm::vec3 position, glm::vec3 axis, float strength);
        BlackholeSetup makeBlackHole(glm::vec3 position, float strength);

    private:
        void createParticleSSBO();
        void createEmitterSSBOs();
        void createForceSSBOs();
        void createFrameUBOs();
        void createDescriptorSetLayouts();
        void createDescriptorSets();
        void createComputePipelineLayout();
        void createGraphicsPipelineLayout();
        void createComputePipeline();
        void createGraphicsPipeline(VkRenderPass renderPass);
        void uploadBuffers(int frameIndex, float deltaTime, float totalTime);
        void insertGraphicsToComputeBarrier(VkCommandBuffer commandBuffer);
        void insertComputeToGraphicsBarrier(VkCommandBuffer commandBuffer);

        SEDevice& seDevice;

        std::vector<Emitter> emitters;
        std::vector<Force>   forces;
        std::vector<bool>    emitterSlotFree;
        std::vector<bool>    forceSlotFree;

        std::unique_ptr<SEPipeline> seComputePipeline;
        std::unique_ptr<SEPipeline> seGraphicsPipeline;

        VkPipelineLayout computePipelineLayout  = VK_NULL_HANDLE;
        VkPipelineLayout graphicsPipelineLayout = VK_NULL_HANDLE;

        VkDescriptorSetLayout computeDescriptorSetLayout  = VK_NULL_HANDLE;
        VkDescriptorSetLayout graphicsDescriptorSetLayout = VK_NULL_HANDLE;

        std::vector<VkDescriptorSet> computeDescriptorSets;
        std::vector<VkDescriptorSet> graphicsDescriptorSets;

        // Buffers
        ParticleBuffer              particleSSBO;
        std::vector<ParticleBuffer> emitterSSBOs;
        std::vector<ParticleBuffer> forceSSBOs;
        std::vector<ParticleBuffer> frameUBOs;
    };

} // namespace se
