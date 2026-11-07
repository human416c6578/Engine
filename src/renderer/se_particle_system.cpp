#include "engine/renderer/se_particle_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cstring>
#include <stdexcept>

namespace se
{
    ParticleSystem::ParticleSystem(SEDevice& device, VkRenderPass renderPass)
        : seDevice{ device }
    {
        // Pre-allocate slot arrays
        emitters.resize(MAX_EMITTERS);
        forces.resize(MAX_FORCES);
        emitterSlotFree.resize(MAX_EMITTERS, true);
        forceSlotFree.resize(MAX_FORCES, true);

        createParticleSSBO();
        createEmitterSSBOs();
        createForceSSBOs();
        createFrameUBOs();

        createDescriptorSetLayouts();
        createDescriptorSets();

        createComputePipelineLayout();
        createGraphicsPipelineLayout();
        createComputePipeline();
        createGraphicsPipeline(renderPass);
    }

    ParticleSystem::~ParticleSystem()
    {
        seComputePipeline.reset();
        seGraphicsPipeline.reset();

        vkDestroyPipelineLayout(seDevice.device(), computePipelineLayout,  nullptr);
        vkDestroyPipelineLayout(seDevice.device(), graphicsPipelineLayout, nullptr);

        vkDestroyDescriptorSetLayout(seDevice.device(), computeDescriptorSetLayout,  nullptr);
        vkDestroyDescriptorSetLayout(seDevice.device(), graphicsDescriptorSetLayout, nullptr);

        auto destroyBuffer = [&](ParticleBuffer& b)
        {
            if (b.mapped)   vkUnmapMemory(seDevice.device(), b.memory);
            if (b.buffer)   vkDestroyBuffer(seDevice.device(), b.buffer, nullptr);
            if (b.memory)   vkFreeMemory(seDevice.device(), b.memory, nullptr);
        };

        destroyBuffer(particleSSBO);
        for (auto& b : emitterSSBOs) destroyBuffer(b);
        for (auto& b : forceSSBOs)   destroyBuffer(b);
        for (auto& b : frameUBOs)    destroyBuffer(b);
    }

    EmitterHandle ParticleSystem::addEmitter(const Emitter& emitter)
    {
        for (uint32_t i = 0; i < MAX_EMITTERS; i++)
        {
            if (emitterSlotFree[i])
            {
                emitters[i]         = emitter;
                emitters[i].id      = i;
                emitters[i].active  = 1;
                emitterSlotFree[i]  = false;

                std::cout<<i<<std::endl;

                return i;
            }
        }
        throw std::runtime_error("ParticleSystem: max emitters reached!");
    }

    void ParticleSystem::removeEmitter(EmitterHandle handle)
    {
        assert(handle < MAX_EMITTERS);
        emitters[handle]        = {};
        emitterSlotFree[handle] = true;
    }

    Emitter& ParticleSystem::getEmitter(EmitterHandle handle)
    {
        assert(handle < MAX_EMITTERS && !emitterSlotFree[handle]);
        return emitters[handle];
    }

    void ParticleSystem::setEmitterActive(EmitterHandle handle, bool active)
    {
        assert(handle < MAX_EMITTERS);
        emitters[handle].active = active ? 1u : 0u;
    }

    ForceHandle ParticleSystem::addForce(const Force& force)
    {
        for (uint32_t i = 0; i < MAX_FORCES; i++)
        {
            if (forceSlotFree[i])
            {
                forces[i]        = force;
                forceSlotFree[i] = false;
                return i;
            }
        }
        throw std::runtime_error("ParticleSystem: max forces reached!");
    }

    void ParticleSystem::removeForce(ForceHandle handle)
    {
        assert(handle < MAX_FORCES);
        forces[handle]        = {};
        forceSlotFree[handle] = true;

        for (auto& e : emitters)
        {
            for (uint32_t i = 0; i < e.forceCount; i++)
            {
                if (e.forceIndices[i] == handle)
                {
                    for (uint32_t j = i; j < e.forceCount - 1; j++)
                        e.forceIndices[j] = e.forceIndices[j + 1];
                    e.forceIndices[--e.forceCount] = ~0u;
                    break;
                }
            }
        }
    }

    Force& ParticleSystem::getForce(ForceHandle handle)
    {
        assert(handle < MAX_FORCES && !forceSlotFree[handle]);
        return forces[handle];
    }

    void ParticleSystem::attachForce(EmitterHandle emitterHandle, ForceHandle forceHandle)
    {
        auto& e = getEmitter(emitterHandle);
        assert(e.forceCount < 8 && "Max 8 forces per emitter");
        e.forceIndices[e.forceCount++] = forceHandle;
    }

    void ParticleSystem::detachForce(EmitterHandle emitterHandle, ForceHandle forceHandle)
    {
        auto& e = getEmitter(emitterHandle);
        for (uint32_t i = 0; i < e.forceCount; i++)
        {
            if (e.forceIndices[i] == forceHandle)
            {
                for (uint32_t j = i; j < e.forceCount - 1; j++)
                    e.forceIndices[j] = e.forceIndices[j + 1];
                e.forceIndices[--e.forceCount] = ~0u;
                return;
            }
        }
    }

    ParticleSystem::FireSetup ParticleSystem::makeFire(glm::vec3 position, float scale)
    {
        Emitter e{};
        e.volume.position  = { position, 0.f };
        e.volume.extents   = { 0.3f * scale, 0.f, 0.f, 0.f };
        e.volume.type      = VolumeType::Sphere;
        e.direction        = { 0.f, -1.f, 0.f, 0.5f };  // w = spread
        e.speedMin         = 1.f * scale;
        e.speedMax         = 3.f * scale;
        e.lifetimeMin      = 0.8f;
        e.lifetimeMax      = 2.f;
        e.colorStart       = { 1.f, 0.6f, 0.f, 1.f };
        e.colorEnd         = { 0.8f, 0.f, 0.f, 0.f };
        e.sizeStart        = .8f * scale;
        e.sizeEnd          = .6f;
        e.emitRate         = 800.f * scale;

        Force upDraft{};
        upDraft.direction = { 0.f, -1.f, 0.f, 0.f };
        upDraft.strength  = 1.5f * scale;
        upDraft.type      = ForceType::Directional;

        Force turbulence{};
        turbulence.direction = { 0.f, 1.f, 0.f, 0.f };
        turbulence.strength  = 1.5f * scale;
        turbulence.type      = ForceType::Turbulence;

        FireSetup setup{};
        setup.emitter  = addEmitter(e);
        setup.upDraft  = addForce(upDraft);
        setup.turbulence = addForce(turbulence);
        attachForce(setup.emitter, setup.upDraft);
        return setup;
    }

    ParticleSystem::RainSetup ParticleSystem::makeRain(glm::vec3 position, glm::vec3 area)
    {
        Emitter e{};
        e.volume.position  = { position, 0.f };
        e.volume.extents   = { area, 0.f };
        e.volume.type      = VolumeType::Box;
        e.direction        = { 0.f, 1.f, 0.f, 0.05f };
        e.speedMin         = 2.0f;
        e.speedMax         = 4.0f;
        e.lifetimeMin      = 1.5f;
        e.lifetimeMax      = 2.5f;
        e.colorStart       = { 0.65f, 0.68f, 1.0f, 0.55f };
        e.colorEnd         = { 0.65f, 0.68f, 0.72f, 0.0f };
        e.sizeStart        = .8f;
        e.sizeEnd          = .8f;
        e.emitRate         = 3000.f;

        Force gravity{};
        gravity.direction = { 0.f, 1.f, 0.f, 0.f };
        gravity.strength  = 0.8f;
        gravity.type      = ForceType::Directional;

        Force wind{};
        wind.direction = { 1.f, 0.f, 0.f, 0.f };
        wind.strength  = 0.75f;
        wind.type      = ForceType::Directional;

        RainSetup setup{};
        setup.emitter = addEmitter(e);
        setup.gravity = addForce(gravity);
        setup.wind    = addForce(wind);
        attachForce(setup.emitter, setup.gravity);
        attachForce(setup.emitter, setup.wind);
        return setup;
    }

    ParticleSystem::RainSetup ParticleSystem::makeSnow(glm::vec3 position, glm::vec3 area)
    {
        Emitter e{};
        e.volume.position  = { position, 0.f };
        e.volume.extents   = { area, 0.f };
        e.volume.type      = VolumeType::Box;
        e.direction        = { 0.f, 1.f, 0.f, 0.05f };
        e.speedMin         = 0.2f;
        e.speedMax         = 0.4f;
        e.lifetimeMin      = 4.5f;
        e.lifetimeMax      = 8.5f;
        e.colorStart       = { 0.65f, 0.68f, 0.72f, 0.55f };
        e.colorEnd         = { 0.65f, 0.68f, 0.72f, 0.0f };
        e.sizeStart        = .8f;
        e.sizeEnd          = .8f;
        e.emitRate         = 3000.f;

        Force gravity{};
        gravity.direction = { 0.f, 1.f, 0.f, 0.f };
        gravity.strength  = 0.8f;
        gravity.type      = ForceType::Directional;

        Force wind{};
        wind.direction = { 1.f, 0.f, 0.f, 0.f };
        wind.strength  = 1.5f;
        wind.type      = ForceType::Turbulence;

        RainSetup setup{};
        setup.emitter = addEmitter(e);
        setup.gravity = addForce(gravity);
        setup.wind    = addForce(wind);
        attachForce(setup.emitter, setup.gravity);
        attachForce(setup.emitter, setup.wind);
        return setup;
    }

    ParticleSystem::VortexSetup ParticleSystem::makeVortex(
        glm::vec3 position,
        glm::vec3 axis,
        float strength)
    {
        axis = glm::normalize(axis);

        Emitter e{};
        e.volume.position  = { position, 0.f };

        e.volume.extents   = { 1.5f, 0.f, 0.f, 0.f };
        e.volume.type      = VolumeType::Ring;
        e.volume.direction = { axis, 0.f };

        e.direction        = { axis, 0.3f };

        e.speedMin         = 0.8f;
        e.speedMax         = 2.5f;

        e.lifetimeMin      = 2.0f;
        e.lifetimeMax      = 4.5f;

        e.colorStart       = { 0.6f, 0.6f, 0.65f, 0.9f };
        e.colorEnd         = { 0.2f, 0.2f, 0.25f, 0.0f };

        e.sizeStart        = 2.5f;
        e.sizeEnd          = 0.8f;

        e.emitRate         = 800.f;

        Force vortex{};
        vortex.position  = { position, 0.f };
        vortex.direction = { axis, 0.f };
        vortex.strength  = strength;
        vortex.radius    = 6.0f;
        vortex.falloff   = 1.0f;
        vortex.type      = ForceType::Vortex;

        Force lift{};
        lift.direction = { 0.f, 1.f, 0.f, 0.f };
        lift.strength  = strength * 0.25f;
        lift.type      = ForceType::Directional;

        Force drag{};
        drag.strength = 0.6f;
        drag.type     = ForceType::Drag;

        VortexSetup setup{};
        setup.emitter = addEmitter(e);
        setup.vortex  = addForce(vortex);
        setup.lift    = addForce(lift);
        setup.drag    = addForce(drag);

        attachForce(setup.emitter, setup.vortex);
        attachForce(setup.emitter, setup.lift);
        attachForce(setup.emitter, setup.drag);

        return setup;
    }

    ParticleSystem::BlackholeSetup ParticleSystem::makeBlackHole(
        glm::vec3 position,
        float strength)
    {
        Emitter e{};
        e.volume.position  = { position, 0.f };

        e.volume.extents   = { 8.f, 0.f, 0.f, 0.f };
        e.volume.type      = VolumeType::Sphere;

        e.direction        = { 0.f, 1.f, 0.f, 0.8f };

        e.speedMin         = 0.5f;
        e.speedMax         = 3.0f;

        e.lifetimeMin      = 2.0f;
        e.lifetimeMax      = 6.0f;

        e.colorStart       = { 0.2f, 0.3f, 0.6f, 1.0f };
        e.colorEnd         = { 1.0f, 0.0f, 0.0f, 1.0f };

        e.sizeStart        = 2.5f;
        e.sizeEnd          = 0.2f;

        e.emitRate         = 1500.f;

        Force gravity{};
        gravity.position  = { position, 0.f };
        gravity.strength  = strength * 4.0f;
        gravity.radius    = 10.0f;
        gravity.falloff   = 2.0f;
        gravity.type      = ForceType::Point;

        Force vortex{};
        vortex.position  = { position, 0.f };
        vortex.direction = { 0.f, 1.f, 0.f, 0.f }; // axis Y
        vortex.strength  = strength * 2.5f;
        vortex.type      = ForceType::Vortex;

        Force drag{};
        drag.strength = 1.2f;
        drag.type     = ForceType::Drag;

        Force turbulence{};
        turbulence.direction = { 0.f, 1.f, 0.f, 0.f };
        turbulence.strength  = strength * 0.6f;
        turbulence.type      = ForceType::Turbulence;

        BlackholeSetup setup{};
        setup.emitter    = addEmitter(e);
        setup.gravity    = addForce(gravity);
        setup.vortex     = addForce(vortex);
        setup.drag       = addForce(drag);
        setup.turbulence = addForce(turbulence);

        attachForce(setup.emitter, setup.gravity);
        attachForce(setup.emitter, setup.vortex);
        attachForce(setup.emitter, setup.drag);
        attachForce(setup.emitter, setup.turbulence);

        return setup;
    }

    void ParticleSystem::createParticleSSBO()
    {
        VkDeviceSize bufferSize = sizeof(Particle) * MAX_PARTICLES;

        std::vector<Particle> particles(MAX_PARTICLES);
        for (uint32_t i = 0; i < MAX_PARTICLES; i++)
        {
            particles[i].position   = { 0.f, 0.f, 0.f, 0.f };  // w=0 = dead
            particles[i].velocity   = { 0.f, 0.f, 0.f, 1.f };
            particles[i].colorStart = { 1.f, 1.f, 1.f, 1.f };
            particles[i].colorEnd   = { 1.f, 1.f, 1.f, 0.f };
            particles[i].emitterID  = -1;
        }

        ParticleBuffer staging{};
        seDevice.createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            staging.buffer, staging.memory);

        void* data;
        vkMapMemory(seDevice.device(), staging.memory, 0, bufferSize, 0, &data);
        std::memcpy(data, particles.data(), bufferSize);
        vkUnmapMemory(seDevice.device(), staging.memory);

        seDevice.createBuffer(bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT  |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            particleSSBO.buffer, particleSSBO.memory);

        seDevice.copyBuffer(staging.buffer, particleSSBO.buffer, bufferSize);
        vkDestroyBuffer(seDevice.device(), staging.buffer, nullptr);
        vkFreeMemory(seDevice.device(), staging.memory, nullptr);
    }

    void ParticleSystem::createEmitterSSBOs()
    {
        VkDeviceSize bufferSize = sizeof(Emitter) * MAX_EMITTERS;
        size_t frames = MAX_FRAMES_IN_FLIGHT;
        emitterSSBOs.resize(frames);

        for (size_t i = 0; i < frames; i++)
        {
            seDevice.createBuffer(bufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                emitterSSBOs[i].buffer, emitterSSBOs[i].memory);

            if (vkMapMemory(seDevice.device(), emitterSSBOs[i].memory,
                            0, bufferSize, 0, &emitterSSBOs[i].mapped) != VK_SUCCESS)
                throw std::runtime_error("Failed to map emitter SSBO memory!");
        }
    }

    void ParticleSystem::createForceSSBOs()
    {
        VkDeviceSize bufferSize = sizeof(Force) * MAX_FORCES;
        size_t frames = MAX_FRAMES_IN_FLIGHT;
        forceSSBOs.resize(frames);

        for (size_t i = 0; i < frames; i++)
        {
            seDevice.createBuffer(bufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                forceSSBOs[i].buffer, forceSSBOs[i].memory);

            if (vkMapMemory(seDevice.device(), forceSSBOs[i].memory,
                            0, bufferSize, 0, &forceSSBOs[i].mapped) != VK_SUCCESS)
                throw std::runtime_error("Failed to map force SSBO memory!");
        }
    }

    void ParticleSystem::createFrameUBOs()
    {
        VkDeviceSize bufferSize = sizeof(ParticleFrameUBO);
        size_t frames = MAX_FRAMES_IN_FLIGHT;
        frameUBOs.resize(frames);

        for (size_t i = 0; i < frames; i++)
        {
            seDevice.createBuffer(bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                frameUBOs[i].buffer, frameUBOs[i].memory);

            if (vkMapMemory(seDevice.device(), frameUBOs[i].memory,
                            0, bufferSize, 0, &frameUBOs[i].mapped) != VK_SUCCESS)
                throw std::runtime_error("Failed to map frame UBO memory!");
        }
    }

    void ParticleSystem::createDescriptorSetLayouts()
    {
        // binding 0 — particle SSBO  (rw)
        // binding 1 — emitter SSBO   (r)
        // binding 2 — force SSBO     (r)
        // binding 3 — frame UBO      (r)
        std::array<VkDescriptorSetLayoutBinding, 4> computeBindings{};

        computeBindings[0].binding        = 0;
        computeBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        computeBindings[0].descriptorCount= 1;
        computeBindings[0].stageFlags     = VK_SHADER_STAGE_COMPUTE_BIT;

        computeBindings[1].binding        = 1;
        computeBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        computeBindings[1].descriptorCount= 1;
        computeBindings[1].stageFlags     = VK_SHADER_STAGE_COMPUTE_BIT;

        computeBindings[2].binding        = 2;
        computeBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        computeBindings[2].descriptorCount= 1;
        computeBindings[2].stageFlags     = VK_SHADER_STAGE_COMPUTE_BIT;

        computeBindings[3].binding        = 3;
        computeBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        computeBindings[3].descriptorCount= 1;
        computeBindings[3].stageFlags     = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo computeLayout{};
        computeLayout.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        computeLayout.bindingCount = static_cast<uint32_t>(computeBindings.size());
        computeLayout.pBindings    = computeBindings.data();

        if (vkCreateDescriptorSetLayout(seDevice.device(), &computeLayout,
                                        nullptr, &computeDescriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute descriptor set layout!");

        // binding 0 — particle SSBO (r) — vertex shader reads pos/color/size
        VkDescriptorSetLayoutBinding graphicsBinding{};
        graphicsBinding.binding        = 0;
        graphicsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        graphicsBinding.descriptorCount= 1;
        graphicsBinding.stageFlags     = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo graphicsLayout{};
        graphicsLayout.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        graphicsLayout.bindingCount = 1;
        graphicsLayout.pBindings    = &graphicsBinding;

        if (vkCreateDescriptorSetLayout(seDevice.device(), &graphicsLayout,
                                        nullptr, &graphicsDescriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics descriptor set layout!");
    }

    void ParticleSystem::createDescriptorSets()
    {
        size_t frames = MAX_FRAMES_IN_FLIGHT;
        computeDescriptorSets.resize(frames);
        graphicsDescriptorSets.resize(frames);

        for (size_t i = 0; i < frames; i++)
        {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool     = seDevice.getDescriptorPool();
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts        = &computeDescriptorSetLayout;

            if (vkAllocateDescriptorSets(seDevice.device(), &allocInfo,
                                         &computeDescriptorSets[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to allocate compute descriptor sets!");

            VkDescriptorBufferInfo particleInfo{};
            particleInfo.buffer = particleSSBO.buffer;
            particleInfo.offset = 0;
            particleInfo.range  = sizeof(Particle) * MAX_PARTICLES;

            VkDescriptorBufferInfo emitterInfo{};
            emitterInfo.buffer = emitterSSBOs[i].buffer;
            emitterInfo.offset = 0;
            emitterInfo.range  = sizeof(Emitter) * MAX_EMITTERS;

            VkDescriptorBufferInfo forceInfo{};
            forceInfo.buffer = forceSSBOs[i].buffer;
            forceInfo.offset = 0;
            forceInfo.range  = sizeof(Force) * MAX_FORCES;

            VkDescriptorBufferInfo frameInfo{};
            frameInfo.buffer = frameUBOs[i].buffer;
            frameInfo.offset = 0;
            frameInfo.range  = sizeof(ParticleFrameUBO);

            std::array<VkWriteDescriptorSet, 4> writes{};
            writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            writes[0].dstSet          = computeDescriptorSets[i];
            writes[0].dstBinding      = 0;
            writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[0].descriptorCount = 1;
            writes[0].pBufferInfo     = &particleInfo;

            writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            writes[1].dstSet          = computeDescriptorSets[i];
            writes[1].dstBinding      = 1;
            writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo     = &emitterInfo;

            writes[2] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            writes[2].dstSet          = computeDescriptorSets[i];
            writes[2].dstBinding      = 2;
            writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[2].descriptorCount = 1;
            writes[2].pBufferInfo     = &forceInfo;

            writes[3] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            writes[3].dstSet          = computeDescriptorSets[i];
            writes[3].dstBinding      = 3;
            writes[3].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[3].descriptorCount = 1;
            writes[3].pBufferInfo     = &frameInfo;

            vkUpdateDescriptorSets(seDevice.device(), 4, writes.data(), 0, nullptr);

            allocInfo.pSetLayouts = &graphicsDescriptorSetLayout;
            if (vkAllocateDescriptorSets(seDevice.device(), &allocInfo,
                                         &graphicsDescriptorSets[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to allocate graphics descriptor sets!");

            VkWriteDescriptorSet graphicsWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            graphicsWrite.dstSet          = graphicsDescriptorSets[i];
            graphicsWrite.dstBinding      = 0;
            graphicsWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            graphicsWrite.descriptorCount = 1;
            graphicsWrite.pBufferInfo     = &particleInfo;

            vkUpdateDescriptorSets(seDevice.device(), 1, &graphicsWrite, 0, nullptr);
        }
    }

    void ParticleSystem::createComputePipelineLayout()
    {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts    = &computeDescriptorSetLayout;

        if (vkCreatePipelineLayout(seDevice.device(), &layoutInfo,
                                   nullptr, &computePipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute pipeline layout!");
    }

    void ParticleSystem::createGraphicsPipelineLayout()
    {
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.offset     = 0;
        pushConstant.size       = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount         = 1;
        layoutInfo.pSetLayouts            = &graphicsDescriptorSetLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges    = &pushConstant;

        if (vkCreatePipelineLayout(seDevice.device(), &layoutInfo,
                                   nullptr, &graphicsPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline layout!");
    }

    void ParticleSystem::createComputePipeline()
    {
        seComputePipeline = std::make_unique<SEPipeline>(
            seDevice,
            "shaders/particleComp.spv",
            computePipelineLayout);
    }

    void ParticleSystem::createGraphicsPipeline(VkRenderPass renderPass)
    {
        PipelineConfigInfo cfg{};
        SEPipeline::particlePipelineConfigInfo(cfg);
        cfg.renderPass     = renderPass;
        cfg.pipelineLayout = graphicsPipelineLayout;

        seGraphicsPipeline = std::make_unique<SEPipeline>(
            seDevice,
            "shaders/particleVert.spv",
            "shaders/particleFrag.spv",
            cfg,
            VK_SAMPLE_COUNT_1_BIT);
    }


    void ParticleSystem::uploadBuffers(int frameIndex, float deltaTime, float totalTime)
    {
        std::memcpy(emitterSSBOs[frameIndex].mapped,
                    emitters.data(),
                    sizeof(Emitter) * MAX_EMITTERS);

        std::memcpy(forceSSBOs[frameIndex].mapped,
                    forces.data(),
                    sizeof(Force) * MAX_FORCES);

        uint32_t activeEmitters = 0;
        uint32_t activeForces   = 0;
        for (size_t i = 0; i < MAX_EMITTERS; i++)
            if (!emitterSlotFree[i]) activeEmitters++;
        for (size_t i = 0; i < MAX_FORCES; i++)
            if (!forceSlotFree[i]) activeForces++;

        ParticleFrameUBO ubo{};
        ubo.deltaTime     = deltaTime;
        ubo.totalTime     = totalTime;
        ubo.particleCount = MAX_PARTICLES;
        ubo.emitterCount  = activeEmitters;
        ubo.forceCount    = activeForces;
        std::memcpy(frameUBOs[frameIndex].mapped, &ubo, sizeof(ubo));
    }

    void ParticleSystem::insertGraphicsToComputeBarrier(VkCommandBuffer commandBuffer)
    {
        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = particleSSBO.buffer;
        barrier.offset = 0;
        barrier.size = sizeof(Particle) * MAX_PARTICLES;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 1, &barrier, 0, nullptr);
    }

    void ParticleSystem::insertComputeToGraphicsBarrier(VkCommandBuffer commandBuffer)
    {
        VkBufferMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                                      VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = particleSSBO.buffer;
        barrier.offset              = 0;
        barrier.size                = sizeof(Particle) * MAX_PARTICLES;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            0, 0, nullptr, 1, &barrier, 0, nullptr);
    }

    void ParticleSystem::update(VkCommandBuffer commandBuffer, int frameIndex,
                                float deltaTime, float totalTime)
    {
        insertGraphicsToComputeBarrier(commandBuffer);
        uploadBuffers(frameIndex, deltaTime, totalTime);

        seComputePipeline->bindCompute(commandBuffer);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            computePipelineLayout,
            0, 1, &computeDescriptorSets[frameIndex],
            0, nullptr);

        uint32_t groupCount = (MAX_PARTICLES + 255) / 256;
        vkCmdDispatch(commandBuffer, groupCount, 1, 1);

        insertComputeToGraphicsBarrier(commandBuffer);
    }

    void ParticleSystem::render(VkCommandBuffer commandBuffer, int frameIndex,
                                glm::mat4 projView)
    {
        seGraphicsPipeline->bind(commandBuffer);

        vkCmdPushConstants(
            commandBuffer,
            graphicsPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(glm::mat4), &projView);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipelineLayout,
            0, 1, &graphicsDescriptorSets[frameIndex],
            0, nullptr);

        vkCmdDraw(commandBuffer, MAX_PARTICLES, 1, 0, 0);
    }

} // namespace se
