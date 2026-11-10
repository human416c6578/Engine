#pragma once

#include <glm/vec3.hpp>
#include <span>
#include <utility>
#include <vector>

namespace se
{
    class SECollider;
    class SERigidbody;

    struct WorldSphere
    {
        glm::vec3 center{0.0f};
        float radius = 0.5f;
    };

    struct WorldBox
    {
        glm::vec3 center{0.0f};
        glm::vec3 halfExtents{0.5f};
    };

    struct Contact
    {
        glm::vec3 normal{0.0f};
        float penetration = 0.0f;
        glm::vec3 point{0.0f};
    };

    bool intersect(const WorldSphere& a, const WorldSphere& b, Contact& contact);
    bool intersect(const WorldBox& a, const WorldBox& b, Contact& contact);
    bool intersect(const WorldSphere& a, const WorldBox& b, Contact& contact);
    bool intersect(const WorldBox& a, const WorldSphere& b, Contact& contact);

    struct PhysicsBodyView
    {
        glm::vec3& position; // Borrow position vector from gameobject
        const SECollider& collider;
        SERigidbody* rigidbody = nullptr;
    };

    class SEPhysics
    {
    public:
        SEPhysics() = default;
        SEPhysics(const SEPhysics&) = delete;
        SEPhysics& operator=(const SEPhysics&) = delete;
        SEPhysics(SEPhysics&&) = delete;
        SEPhysics& operator=(SEPhysics&&) = delete;

        void step(std::span<PhysicsBodyView> bodies, float dt);

    private:
        struct GridEntry {
            glm::ivec3 cell;
            std::size_t bodyIndex;
        };

        void buildCandidatePairs(std::span<PhysicsBodyView> bodies);

        glm::vec3 gravity{0.0f, 9.81f, 0.0f};
        static constexpr float cellSize = 0.65f;

        std::vector<std::size_t> dynamicBodies;
        std::vector<std::size_t> staticBodies;
        std::vector<std::size_t> oversizedBodies;
        std::vector<GridEntry> gridEntries;
        std::vector<glm::ivec3> minimumCells;
        std::vector<std::pair<std::size_t, std::size_t>> candidatePairs;
    };
}
