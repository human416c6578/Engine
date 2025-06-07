#pragma once

#include "se_mesh.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>

namespace se
{

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

        static SEGameObject createGameObject()
        {
            return SEGameObject{ currentId++, std::string("GameObject_") + std::to_string(currentId) };
        }


        static SEGameObject createGameObject(const std::string& name)
        {
            if (name.empty())
                return SEGameObject{ currentId++, std::string("GameObject_") + std::to_string(currentId) };
            return SEGameObject{ currentId++, name };
        }


        //SEGameObject(const SEGameObject &) = delete;
        //SEGameObject &operator=(const SEGameObject &) = delete;
        //SEGameObject(SEGameObject &&) = default;
        //SEGameObject &operator=(SEGameObject &&) = default;

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

        const glm::vec3& getColor() const { return color; }
        void setColor(const glm::vec3& newColor)
        {
            color = newColor;
        }

        const glm::mat4& getTransformMat4() { return transform.mat4(); }

        const TransformComponent& getTransform() const { return transform; }

        void setTransform(const TransformComponent& newTransform)
        {
            transform = newTransform;
        }


    private:
        static id_t currentId;

        SEGameObject(id_t objId, std::string objName) : id{ objId }, name{ objName } {}

        id_t id;
        std::string name;

        std::shared_ptr<SEMesh> mesh{};
        std::shared_ptr<SEMaterial> material{};
        glm::vec3 color{};
        TransformComponent transform{};
    };
}
