#include "glm/ext/vector_float3.hpp"

namespace se
{
    class SERigidbody
    {
        public:
            bool isStatic = false;
            bool useGravity = true;

            float mass = 1.0f;
            glm::vec3 velocity{0.0f};

            float getInverseMass() const noexcept
            {
                return isStatic ? 0.0f : 1.0f / mass;
            }
    };
}