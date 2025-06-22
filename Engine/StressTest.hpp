#pragma once
#include "se_gameobject.hpp"
#include "se_script_component.hpp"
#include "se_script_manager.hpp"
#include "se_input_system.hpp"
#include "se_scene_manager.hpp"

namespace se {

    class StressScript : public ScriptComponent {
    public:

        void onCreate() override {
            sceneManager = &se::SceneManager::getInstance();
            scene = sceneManager->getActiveScene();
            resourceManager = sceneManager->getResourceManager();

            spawnCubes();
        }

        void spawnCubes()
        {
            int count = 30;
            auto mesh = owner->getMesh();
            auto mat = owner->getMaterial();

            for (int i = 0; i < count; ++i) {
                for (int j = 0; j < count; ++j) {

                    auto go = scene->createGameObject("Sphere_" + std::to_string(i) + "_" + std::to_string(j));

                    go->setMesh(mesh);
                    go->setMaterial(mat);

                    float spacing = 5.0f;
                    go->getTransform().translation = glm::vec3(i * spacing, 0.0f, j * spacing);
                    go->getTransform().scale = { 2.0, 2.0, 2.0 };
                }
            }
        }

        void onUpdate(float dt) override {
            if (owner) {

            }
        }

        std::string getName() const override { return "StressScript"; }
    private:
        se::SceneManager* sceneManager{ nullptr };
        se::Scene* scene{ nullptr };
        se::ResourceManager* resourceManager{ nullptr };
    };


}

// Auto-register the script at load time
namespace {
    const bool registered_StressScript = se::registerScript<se::StressScript>("StressScript");
}
