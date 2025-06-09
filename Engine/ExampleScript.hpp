#pragma once
#include "se_gameobject.hpp"
#include "se_script_component.hpp"
#include "se_script_manager.hpp"

namespace se {

    class ExampleScript : public ScriptComponent {
    public:
        void onCreate() override {
            // Initialization logic
        }

        void onUpdate(float dt) override {
            if (owner) {
                auto& transform = owner->getTransform();
                transform.rotation.y += dt;
            }
        }

        std::string getName() const override { return "ExampleScript"; }
    };

}

// Auto-register the script at load time
namespace {
    const bool registered_ExampleScript = se::registerScript<se::ExampleScript>("ExampleScript");
}
