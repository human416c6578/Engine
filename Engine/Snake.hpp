#pragma once
#include "se_gameobject.hpp"
#include "se_script_component.hpp"
#include "se_script_manager.hpp"
#include "se_input_system.hpp"
#include "se_scene_manager.hpp"
#include "se_gameobject_handle.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <string>

namespace se {

    class SnakeScript : public ScriptComponent {
    public:
        float stepInterval = 0.2f;
        float stepTimer = 0.0f;
        float tileSize = 1.0f;
        bool gameOver = false;
        glm::vec3 dir;
        std::vector<GameObjectHandle> body;
        GameObjectHandle apple;

        void onCreate() override {
            sceneManager = &SceneManager::getInstance();
            scene = sceneManager->getActiveScene();
            resourceManager = sceneManager->getResourceManager();

            dir = glm::vec3(0.0f);

            body.push_back(GameObjectHandle(owner->getId(), scene));

            apple = scene->getGameObjectByName("apple");
            if (!apple) {
                apple = scene->createGameObject("apple");
                apple->getTransform().translation = getRandomPosition();
            }

        }

        void onUpdate(float dt) override {
            if (!owner) return;
            
            handleInput();
            

            if (gameOver) return;

            stepTimer += dt;

            if (stepTimer >= stepInterval) {
                stepTimer = 0.0f;

                moveBody();

                auto& headTransform = owner->getTransform();
                headTransform.translation += dir * tileSize;

                wrapAroundBounds();

                if (checkSelfCollision()) {
                    gameOver = true;
                    return;
                }

                checkAppleCollision();
            }
        }


        void handleInput() {
            if (SEInputSystem::isKeyPressed(GLFW_KEY_LEFT) && dir.x == 0)
                dir = glm::vec3(-1.0f, 0.0f, 0.0f);
            if (SEInputSystem::isKeyPressed(GLFW_KEY_RIGHT) && dir.x == 0)
                dir = glm::vec3(1.0f, 0.0f, 0.0f);
            if (SEInputSystem::isKeyPressed(GLFW_KEY_UP) && dir.z == 0)
                dir = glm::vec3(0.0f, 0.0f, 1.0f);
            if (SEInputSystem::isKeyPressed(GLFW_KEY_DOWN) && dir.z == 0)
                dir = glm::vec3(0.0f, 0.0f, -1.0f);
            if (SEInputSystem::isKeyPressed(GLFW_KEY_ESCAPE))
            {
                resetScene();
            }
        }

        void moveBody() {
            for (int i = static_cast<int>(body.size()) - 1; i > 0; --i) {
                if (body[i] && body[i - 1])
                    body[i]->getTransform().translation = body[i - 1]->getTransform().translation;
            }
        }

        bool checkSelfCollision() {
            auto headPos = owner->getTransform().translation;

            for (size_t i = 1; i < body.size(); ++i) {
                if (!body[i]) continue;
                auto segmentPos = body[i]->getTransform().translation;
                if (glm::distance(headPos, segmentPos) < tileSize * 0.5f) {
                    return true;
                }
            }
            return false;
        }


        void checkAppleCollision() {
            
            if (!apple) return;

            auto headPos = owner->getTransform().translation;
            auto applePos = apple->getTransform().translation;

            if (glm::distance(headPos, applePos) <= tileSize * 0.5f){
                growBody();
                apple->getTransform().translation = getRandomPosition();
            }
        }

        void wrapAroundBounds() {
            auto& pos = owner->getTransform().translation;

            const float maxX = 10.0f * tileSize;
            const float maxZ = 10.0f * tileSize;
            const float minX = -10.0f * tileSize;
            const float minZ = -10.0f * tileSize;

            if (pos.x < minX) pos.x = maxX - tileSize;
            else if (pos.x >= maxX) pos.x = minX;

            if (pos.z < minZ) pos.z = maxZ - tileSize;
            else if (pos.z >= maxZ) pos.z = minZ;
        }

        void resetScene()
        {
            destroyBody();
            dir = glm::vec3(0.0f, 0.0f, 0.0f);
            stepTimer = 0.0f;
            gameOver = false;
        }

        void growBody() {
            auto newSegment = scene->createGameObject("segment_" + std::to_string(body.size()));
            newSegment->getTransform().translation = body.back()->getTransform().translation;
            newSegment->setMesh(owner->getMesh());
            newSegment->setMaterial(owner->getMaterial());
            body.push_back(newSegment);
        }

        glm::vec3 getRandomPosition() {
            glm::vec3 pos;
            bool valid = false;

            while (!valid) {
                int x = rand() % 10;
                int z = rand() % 10;
                pos = glm::vec3(static_cast<float>(x) * tileSize, 0.0f, static_cast<float>(z) * tileSize);

                valid = true;
                // Check against snake body
                for (auto& segment : body) {
                    if (!segment) continue;
                    if (glm::distance(segment->getTransform().translation, pos) < tileSize * 0.5f) {
                        valid = false;
                        break;
                    }
                }
            }

            return pos;
        }


        std::string getName() const override { return "SnakeScript"; }

        void destroyBody() {
            if (body.empty()) return;

            GameObjectHandle head = body[0];

            for (size_t i = 1; i < body.size(); ++i) {
                if (body[i]) {
                    body[i].destroy();
                }
            }

            body.clear();
            body.push_back(head);

            if (head) {
                head->getTransform().translation = glm::vec3(0.0f);
            }
        }

    private:
        SceneManager* sceneManager = nullptr;
        Scene* scene = nullptr;
        ResourceManager* resourceManager = nullptr;
    };

}

namespace {
    const bool registered_SnakeScript = se::registerScript<se::SnakeScript>("SnakeScript");
}
