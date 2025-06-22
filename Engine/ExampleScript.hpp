#pragma once
#include "se_gameobject.hpp"
#include "se_script_component.hpp"
#include "se_script_manager.hpp"

namespace se {

    class ExampleScript : public ScriptComponent {
    public:
        void onCreate() override {
            sceneManager = &SceneManager::getInstance();
            scene = sceneManager->getActiveScene();
            resourceManager = sceneManager->getResourceManager();

            spawnSpheres();
        }

        void spawnSpheres()
        {
            float step = 0.1f;
            int count = static_cast<int>(1.0f / step) + 1;

            for (int i = 0; i < count; ++i) {
                float metallic = i * step;
                for (int j = 0; j < count; ++j) {
                    float roughness = j * step;

                    auto go = scene->createGameObject("Sphere_" + std::to_string(i) + "_" + std::to_string(j));
                    auto mesh = resourceManager->createSphere("SphereMesh");
                    auto mat = resourceManager->createMaterial("Mat_" + std::to_string(i) + "_" + std::to_string(j));


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

        void onUpdate(float dt) override {
            if (owner) {
                auto& transform = owner->getTransform();
                transform.rotation.y += dt;
            }
        }

        std::string getName() const override { return "ExampleScript"; }

    private:
        SceneManager* sceneManager = nullptr;
        Scene* scene = nullptr;
        ResourceManager* resourceManager = nullptr;
    };

}

// Auto-register the script at load time
namespace {
    const bool registered_ExampleScript = se::registerScript<se::ExampleScript>("ExampleScript");
}
