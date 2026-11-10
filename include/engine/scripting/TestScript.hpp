#pragma once
#include "engine/scene/se_gameobject.hpp" // Inline callbacks access game object components.
#include "engine/scripting/se_script_component.hpp" // TestScript derives from ScriptComponent.
#include "engine/platform/se_input_system.hpp" // Inline callbacks query input.
#include <GLFW/glfw3.h>
#include "engine/scene/se_scene.hpp" // Inline callbacks use Scene operations.

namespace se {

    class TestScript : public ScriptComponent{
    public:
        float speed = 10.0f;

        void onUpdate(float dt, ScriptContext &context) override {
            if (owner) {
                auto& transform = owner->getTransform();
                auto rigidbody = owner->getRigidbody();

                if (context.input.isKeyPressed(GLFW_KEY_LEFT))
                {
                    transform.translation.x += dt * speed;
                }
                if (context.input.isKeyPressed(GLFW_KEY_RIGHT))
                {
                    transform.translation.x -= dt * speed;
                }
                if (context.input.isKeyPressed(GLFW_KEY_UP))
                {
                    transform.translation.z -= dt * speed;
                }
                if (context.input.isKeyPressed(GLFW_KEY_DOWN))
                {
                    transform.translation.z += dt * speed;
                }
                if (context.input.isKeyPressed(GLFW_KEY_LEFT_CONTROL))
                {
                    if(rigidbody == nullptr) return;
                    if(rigidbody->velocity.y > 0.01f || rigidbody->velocity.y < -0.01f)
                        return;
                    rigidbody->velocity.y = -speed / 3.0f;
                }
            }
        }

        std::string getName() const override { return "TestScript"; }
    private:
        bool created{ false };
    };


}
