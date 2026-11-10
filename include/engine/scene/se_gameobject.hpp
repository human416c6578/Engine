#pragma once

#include "engine/scene/se_particle_emitter_component.hpp" // Stored by value in SEGameObject.
#include "engine/scripting/se_script_component.hpp" // Inline script lifecycle methods need the complete base type.
#include "engine/physics/se_collider.hpp"
#include "engine/physics/se_rigidbody.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <optional>

namespace se
{
    class SEMaterial;
    class SEMesh;

    enum class LightType {
        None = 0,
        Point = 1,
        Directional = 2,
        Spot = 3
    };

    struct alignas(16) Light {
        glm::vec3 position;
        alignas(4) LightType type;
        glm::vec3 color;
        alignas(4) float intensity;
        glm::vec3 direction;
        alignas(4) float spotAngle;
    };

    struct TransformComponent
    {
        glm::vec3 translation{};
        glm::vec3 scale{ 1.f, 1.f, 1.f };
        glm::vec3 rotation{};

        // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
        // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
        glm::mat4 mat4()
        {
            const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);
            return glm::mat4{
                {
                    scale.x * (c1 * c3 + s1 * s2 * s3),
                    scale.x * (c2 * s3),
                    scale.x * (c1 * s2 * s3 - c3 * s1),
                    0.0f,
                },
                {
                    scale.y * (c3 * s1 * s2 - c1 * s3),
                    scale.y * (c2 * c3),
                    scale.y * (c1 * c3 * s2 + s1 * s3),
                    0.0f,
                },
                {
                    scale.z * (c2 * s1),
                    scale.z * (-s2),
                    scale.z * (c1 * c2),
                    0.0f,
                },
                {translation.x, translation.y, translation.z, 1.0f} };
        }
    };

    class SEGameObject
    {
    public:
        using id_t = unsigned int;

        SEGameObject() : id{ 0 }, name{ "GameObject_0" } {}

        explicit SEGameObject(const std::string& name) : id{ 0 }, name{ name } {}

        SEGameObject(id_t objId, std::string objName) : id{ objId }, name{ std::move(objName) } {}

        static SEGameObject createGameObject()
        {
            return SEGameObject{ 0, "GameObject_0" };
        }


        static SEGameObject createGameObject(const std::string& name)
        {
            return SEGameObject{ 0, name.empty() ? "GameObject_0" : name };
        }

        template<typename T, typename... Args>
        void addScript(ScriptContext &context, Args&&... args) {
            static_assert(std::is_base_of<ScriptComponent, T>::value, "Script must inherit from ScriptComponent");
            setScript(std::make_unique<T>(std::forward<Args>(args)...), context);
        }

        void onUpdate(float dt, ScriptContext &context) {
            if (script) script->onUpdate(dt, context);
        }

        void onDestroy() {
            if (script) script->onDestroy();
        }

        SEGameObject(const SEGameObject &) = delete;
        SEGameObject &operator=(const SEGameObject &) = delete;
        SEGameObject(SEGameObject &&) = delete;
        SEGameObject &operator=(SEGameObject &&) = delete;

        id_t getId() { return id; }
        const std::string& getName() const { return name; }
        void setName(const std::string& newName) { name = newName; }

        std::shared_ptr<SEMesh> getMesh() const { return mesh; }
        void setMesh(std::shared_ptr<SEMesh> newMesh)
        {
            mesh = newMesh;
        }

        std::shared_ptr<SEMaterial> getMaterial() const { return material; }
        void setMaterial(std::shared_ptr<SEMaterial> newMaterial)
        {
            material = newMaterial;
        }

        const glm::mat4 getTransformMat4() { return transform.mat4(); }

        const TransformComponent& getTransform() const { return transform; }
        TransformComponent& getTransform() { return transform; }

        void setTransform(const TransformComponent newTransform)
        {
            transform = newTransform;
        }

        SECollider* getCollider() noexcept
        {
            return collider ? &*collider : nullptr;
        }

        const SECollider* getCollider() const noexcept
        {
            return collider ? &*collider : nullptr;
        }

        bool hasCollider() const noexcept
        {
            return collider.has_value();
        }

        void setCollider(const SECollider& value)
        {
            collider = value;
        }

        void removeCollider() noexcept
        {
            collider.reset();
        }

        SERigidbody* getRigidbody() noexcept
        {
            return rigidbody ? &*rigidbody : nullptr;
        }

        const SERigidbody* getRigidbody() const noexcept
        {
            return rigidbody ? &*rigidbody : nullptr;
        }

        bool hasRigidbody() const noexcept
        {
            return rigidbody.has_value();
        }

        void setRigidbody(const SERigidbody& value)
        {
            rigidbody = value;
        }

        void removeRigidbody() noexcept
        {
            rigidbody.reset();
        }

        const Light& getLight() const { return light; }
        Light& getLight() { return light; }

		void setLight(Light newLight)
		{
			light = newLight;
		}

        bool hasLight() const {
            return light.type != LightType::None;
        }

		void setScript(std::unique_ptr<ScriptComponent> newScript, ScriptContext &context)
		{
			script = std::move(newScript);
			if (script) {
				script->setOwner(this);
				script->onCreate(context);
			}
		}

		std::unique_ptr <ScriptComponent>& getScript() {
			return script;
		}

	    const ParticleEmitterComponent& getParticleEmitter() const { return particleEmitter; }
        ParticleEmitterComponent& getParticleEmitter() { return particleEmitter; }
        void setParticleEmitter(const ParticleEmitterComponent& comp) { particleEmitter = comp; }
        bool hasParticleEmitter() const { return particleEmitter.isValid(); }


        id_t id;

    private:
        std::string name;

        std::shared_ptr<SEMesh> mesh{};
        std::shared_ptr<SEMaterial> material{};
        Light light{};
        TransformComponent transform{};

        std::unique_ptr<ScriptComponent> script{nullptr};
        ParticleEmitterComponent particleEmitter{};

        std::optional<SECollider> collider;
        std::optional<SERigidbody> rigidbody;

    };
}
