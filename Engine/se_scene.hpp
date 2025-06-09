#pragma once
#include <vector>
#include <memory>
#include <string>
#include "se_gameobject.hpp"

namespace se {
    class Scene {
    public:
        Scene(const std::string& name) : name(name) {}

        SEGameObject& createGameObject(const std::string& objName) {
            auto obj = std::make_unique<SEGameObject>(SEGameObject::createGameObject(objName));
            SEGameObject& ref = *obj;
            gameObjects.push_back(std::move(obj));
            return ref;
        }

        void onUpdate(float dt) {
            for (auto& obj : gameObjects) {
                obj->onUpdate(dt);
            }
        }

        void onDestroy() {
            for (auto& obj : gameObjects) {
                obj->onDestroy();
            }
            gameObjects.clear();
        }

        // Add methods to find objects, manage camera, etc.

        const std::string& getName() const { return name; }
        std::vector<std::unique_ptr<SEGameObject>>& getGameObjects() { return gameObjects; }


    private:
        std::string name;
        std::vector<std::unique_ptr<SEGameObject>> gameObjects;
    };

} // namespace se
