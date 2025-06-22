#pragma once
#include "se_gameobject.hpp"
#include "se_script_component.hpp"
#include "se_script_manager.hpp"
#include "se_input_system.hpp"
#include "se_scene_manager.hpp"

namespace se {

    class TestScript : public ScriptComponent{
    public:
        float speed = 10.0f;

        void onCreate() override {
            sceneManager = &se::SceneManager::getInstance();
            scene = sceneManager->getActiveScene();
            resourceManager = sceneManager->getResourceManager();
        }

        void onUpdate(float dt) override {
            if (owner) {
                auto& transform = owner->getTransform();

                if (se::SEInputSystem::isKeyPressed(GLFW_KEY_LEFT))
                {
                    transform.translation.x += dt * speed;
                }
                if (se::SEInputSystem::isKeyPressed(GLFW_KEY_RIGHT))
                {
                    transform.translation.x -= dt * speed;
                }
                if (se::SEInputSystem::isKeyPressed(GLFW_KEY_UP))
                {
                    transform.translation.z -= dt * speed;
                }
                if (se::SEInputSystem::isKeyPressed(GLFW_KEY_DOWN))
                {
                    transform.translation.z += dt * speed;
                }

                if (se::SEInputSystem::isKeyPressed(GLFW_KEY_F) && !created)
                {
                    scene->createGameObject("Test");
                    created = true;
                }
            }
        }

        std::string getName() const override { return "TestScript"; }
    private:
        se::SceneManager* sceneManager{ nullptr };
        se::Scene* scene{ nullptr };
        se::ResourceManager* resourceManager{ nullptr };
        bool created{ false };
    };


}

// Auto-register the script at load time
namespace {
    const bool registered_TestScript = se::registerScript<se::TestScript>("TestScript");
}
