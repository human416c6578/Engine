#include "glm/ext/vector_float3.hpp"
#include <variant>
namespace se
{

    struct BoxShape
    {
        glm::vec3 halfExtents{0.5f};
    };

    struct SphereShape
    {
        float radius = 0.5f;
    };

    // Local Y axis, centered on the collider origin. Collision uses the
    // enclosing AABB, not the curved cylinder surface.
    struct CylinderShape
    {
        float radius = 0.5f;
        float halfHeight = 0.5f;
    };

    using ColliderShape = std::variant<BoxShape, SphereShape, CylinderShape>;

    class SECollider
    {
        public:
            ColliderShape shape{BoxShape{}};

            glm::vec3 localOffset{0.0f};

            float friction = 0.5f;
    };
}