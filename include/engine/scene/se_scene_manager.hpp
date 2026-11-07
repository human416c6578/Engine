#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "engine/scene/se_scene.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/renderer/se_postprocess.hpp"
#include "engine/renderer/se_particle_system.hpp"
#include "engine/core/se_camera.hpp"

namespace se {

    class SceneManager {
    public:
        static SceneManager& getInstance() {
            static SceneManager instance;
            return instance;
        }

        void destroyAllScenes()
        {
            activeScene = nullptr;
            scenes.clear();
        }

        void setResourceManager(ResourceManager* rm)
        {
            resourceManager = rm;
        }

        void setPostProcess(SEPostProcess* pp)       { postProcess = pp; }
        void setParticleSystem(ParticleSystem* ps)   { particleSystem = ps; }

        ResourceManager* getResourceManager()
        {
            return resourceManager;
        }

        Scene& createScene(const std::string& name)
        {
            auto scene = std::make_unique<Scene>(name);
            Scene& ref = *scene;
            scenes[name] = std::move(scene);
            return ref;
        }

        Scene* getScene(const std::string& name)
        {
            auto it = scenes.find(name);
            return it != scenes.end() ? it->second.get() : nullptr;
        }

        void setActiveScene(const std::string& name)
        {
            activeScene = getScene(name);
        }

        Scene* getActiveScene() { return activeScene; }

        SECamera& getCamera() { return activeScene->getCamera(); }

        bool saveActiveScene(const std::string& filePath);

        bool saveScene(const std::string& sceneName, const std::string& filePath);

        bool loadScene(const std::string& filePath, const std::string& sceneName);


    private:
        ResourceManager* resourceManager = nullptr;
        SEPostProcess*   postProcess   = nullptr;
        ParticleSystem*  particleSystem = nullptr;

        std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
        Scene* activeScene = nullptr;

    };

} // namespace se
