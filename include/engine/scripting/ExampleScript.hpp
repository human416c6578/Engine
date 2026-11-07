#pragma once
#include "engine/scene/se_gameobject.hpp"
#include "engine/scene/se_scene.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/scripting/se_script_component.hpp"

namespace se {

    class ExampleScript : public ScriptComponent {
    public:
        void onCreate(ScriptContext &context) override {
            spawnSpheres(context);
        }

        void spawnSpheres(ScriptContext &context)
        {
            float step = 0.1f;
            int count = static_cast<int>(1.0f / step) + 1;

            for (int i = 0; i < count; ++i) {
                float metallic = i * step;
                for (int j = 0; j < count; ++j) {
                    float roughness = j * step;

                    auto go = context.scene.createGameObject("Sphere_" + std::to_string(i) + "_" + std::to_string(j));
                    auto mesh = context.resources.createSphere("SphereMesh");
                    auto mat = context.resources.createMaterial("Mat_" + std::to_string(i) + "_" + std::to_string(j));


                    mat->setColor({ 0.0f, 0.7f, 1.0f });
                    mat->setMetallic(metallic);
                    mat->setRoughness(roughness);
                    mat->setAO(1.0f);

                    go->setMesh(mesh);
                    go->setMaterial(mat);

                    float spacing = 0.8f;
                    go->getTransform().translation = glm::vec3(i * spacing, j * spacing - 11.0f, -11.0f);
                    go->getTransform().scale = { 0.4, 0.4, 0.4 };
                }
            }
        }

        void onUpdate(float dt, ScriptContext &) override {
            if (owner) {
                auto& transform = owner->getTransform();
                transform.rotation.y += dt;
            }
        }

        std::string getName() const override { return "ExampleScript"; }

    };

}
