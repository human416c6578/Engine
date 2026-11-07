#pragma once
#include "engine/scene/se_gameobject.hpp"
#include "engine/scripting/se_script_component.hpp"
#include "engine/scene/se_scene.hpp"

namespace se {

    class StressScript : public ScriptComponent {
    public:

        void onCreate(ScriptContext &context) override {
            spawnCubes(context);
        }

        void spawnCubes(ScriptContext &context)
        {
            int count = 30;
            auto mesh = owner->getMesh();
            auto mat = owner->getMaterial();

            for (int i = 0; i < count; ++i) {
                for (int j = 0; j < count; ++j) {

                    auto go = context.scene.createGameObject("Sphere_" + std::to_string(i) + "_" + std::to_string(j));

                    go->setMesh(mesh);
                    go->setMaterial(mat);

                    float spacing = 5.0f;
                    go->getTransform().translation = glm::vec3(i * spacing, 0.0f, j * spacing);
                    go->getTransform().scale = { 2.0, 2.0, 2.0 };
                }
            }
        }

        void onUpdate(float, ScriptContext &) override {
            if (owner) {

            }
        }

        std::string getName() const override { return "StressScript"; }
    };


}
