#pragma once

#include "engine/core/se_logger.hpp"
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include "engine/core/se_camera.hpp" // SECamera is stored by value.
#include "engine/physics/se_physics.hpp"
#include "engine/scene/se_gameobject.hpp" // Inline scene operations access complete SEGameObject objects.
#include "engine/scene/se_gameobject_handle.hpp" // GameObjectHandle is constructed and returned inline.

namespace se {
    class ResourceManager;
    class SEInputSystem;

    class Scene {
    public:
        Scene(const std::string& name) : name(name) {}

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
                    (*it)->onDestroy();
                    (*it)->getParticleEmitter().release();
                    gameObjects.erase(it);
                    break;
                }
            }
        }

        ScriptContext scriptContext(ResourceManager& resources, SEInputSystem& input) {
            return ScriptContext{*this, resources, input};
        }

        // Run structural scene changes only after script iteration finishes.
        void defer(std::function<void()> command) {
            pendingCommands.push_back(std::move(command));
        }

        void clearGameObjectsExcept(SEGameObject::id_t survivorId) {
            for (auto it = gameObjects.begin(); it != gameObjects.end();) {
                if (*it && (*it)->getId() == survivorId) {
                    ++it;
                    continue;
                }
                if (*it) {
                    (*it)->onDestroy();
                    (*it)->getParticleEmitter().release();
                }
                it = gameObjects.erase(it);
            }
        }

        void onUpdate(float dt, ResourceManager &resources, SEInputSystem &input) {
            ScriptContext context = scriptContext(resources, input);
            for (auto& obj : gameObjects) {
                if (!obj) {
                    log::error("Scene", "Null object in gameObjects!");
                    continue;
                }
                obj->onUpdate(dt, context);
            }
            // Swap so commands queued by callbacks wait until the next update.
            std::vector<std::function<void()>> commands;
            commands.swap(pendingCommands);
            for (auto& command : commands) command();
        }

        void onFixedUpdate(float dt) {
            std::vector<se::PhysicsBodyView> bodies;

            for(auto& obj : gameObjects)
            {
                if(!obj->getCollider())
                    continue;
                
                auto* body = obj->getRigidbody();
                bodies.push_back({
                    obj->getTransform().translation,
                    *obj->getCollider(),
                    body
                });
            }
            physics.step(bodies, dt);
        }

        void onDestroy() {
            for (auto& obj : gameObjects) {
                obj->onDestroy();
            }
            gameObjects.clear();
        }

        std::size_t getColliderCount() const noexcept {
            std::size_t count = 0;
            for (const auto& object : gameObjects)
                if (object && object->getCollider()) ++count;
            return count;
        }

        std::size_t getDynamicBodyCount() const noexcept {
            std::size_t count = 0;
            for (const auto& object : gameObjects) {
                if (!object || !object->getCollider()) continue;
                const auto* body = object->getRigidbody();
                if (body && !body->isStatic) ++count;
            }
            return count;
        }

        // Monotonic within this scene; lets App notice same-size test rebuilds.
        SEGameObject::id_t getObjectCreationCount() const noexcept { return nextGameObjectId - 1; }

        // Add methods to find objects, manage camera, etc.
        SECamera& getCamera() { return camera; }
        const std::string& getName() const { return name; }
        std::list<std::unique_ptr<SEGameObject>>& getGameObjects() { return gameObjects; }


    private:
        std::string name;
        se::SECamera camera{};
        se::SEPhysics physics{};
        SEGameObject::id_t nextGameObjectId{1};
        std::list<std::unique_ptr<SEGameObject>> gameObjects;
        std::vector<std::function<void()>> pendingCommands;

    };

} // namespace se
