#pragma once

#include "engine/core/se_logger.hpp"
#include <list>
#include <memory>
#include <string>
#include "engine/core/se_camera.hpp" // SECamera is stored by value.
#include "engine/scene/se_gameobject.hpp" // Inline scene operations access complete SEGameObject objects.
#include "engine/scene/se_gameobject_handle.hpp" // GameObjectHandle is constructed and returned inline.

namespace se {
    class ResourceManager;
    class SEInputSystem;

    class Scene {
    public:
        Scene(const std::string& name, Logger& log) : logger{log}, name(name) {}

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) = delete;
        Scene& operator=(Scene&&) = delete;

        GameObjectHandle createGameObject(const std::string& name) {
            auto obj = std::make_unique<SEGameObject>(nextGameObjectId++, name);
            const int id = static_cast<int>(obj->id);
            gameObjects.push_back(std::move(obj));
            return GameObjectHandle(id, this);
        }

        SEGameObject& createGameObjectRef(const std::string& name) {
            auto obj = std::make_unique<SEGameObject>(nextGameObjectId++, name);
            gameObjects.push_back(std::move(obj));
            return *gameObjects.back();
        }

        GameObjectHandle getGameObjectByName(const std::string& objName) {
            for (auto& obj : gameObjects) {
                if (obj && obj->getName() == objName)
                    return GameObjectHandle(obj->id, this);
            }
            return GameObjectHandle();
        }

        SEGameObject* getGameObjectById(int id) {
            for (auto& obj : gameObjects) {
                if (obj && obj->id == id)
                    return obj.get();
            }
            return nullptr;
        }

        void destroyGameObject(int id)
        {
            for (auto it = gameObjects.begin(); it != gameObjects.end(); ++it) {
                if (*it && (*it)->id == id) {
                    gameObjects.erase(it);
                    break;
                }
            }
        }

        ScriptContext scriptContext(ResourceManager& resources, SEInputSystem& input) {
            return ScriptContext{*this, resources, input, logger};
        }

        void onUpdate(float dt, ResourceManager &resources, SEInputSystem &input) {
            ScriptContext context = scriptContext(resources, input);
            for (auto& obj : gameObjects) {
                if (!obj) {
                    logger.log(LogLevel::Error, "Scene", "Null object in gameObjects!");
                    continue;
                }
                obj->onUpdate(dt, context);
            }
        }

        void onDestroy() {
            for (auto& obj : gameObjects) {
                obj->onDestroy();
            }
            gameObjects.clear();
        }

        // Add methods to find objects, manage camera, etc.
        SECamera& getCamera() { return camera; }
        const std::string& getName() const { return name; }
        std::list<std::unique_ptr<SEGameObject>>& getGameObjects() { return gameObjects; }


    private:
        Logger& logger;
        std::string name;
        se::SECamera camera{};
        SEGameObject::id_t nextGameObjectId{1};
        std::list<std::unique_ptr<SEGameObject>> gameObjects;

    };

} // namespace se
