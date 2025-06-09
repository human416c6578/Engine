#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "se_scene.hpp"

namespace se {

    class SceneManager {
    public:
        static SceneManager& getInstance() {
            static SceneManager instance;
            return instance;
        }

        Scene& createScene(const std::string& name) {
            auto scene = std::make_unique<Scene>(name);
            Scene& ref = *scene;
            scenes[name] = std::move(scene);
            return ref;
        }

        Scene* getScene(const std::string& name) {
            auto it = scenes.find(name);
            return it != scenes.end() ? it->second.get() : nullptr;
        }

        void setActiveScene(const std::string& name) {
            activeScene = getScene(name);
        }

        Scene* getActiveScene() { return activeScene; }

    private:
        std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
        Scene* activeScene = nullptr;
    };

} // namespace se
