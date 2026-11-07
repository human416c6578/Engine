#pragma once
#include <unordered_map>
#include <memory>
#include <string>

namespace se {
    class Scene;
    class SECamera;
    class ResourceManager;
    class SEPostProcess;
    class ParticleSystem;
    class SEInputSystem;
    class ScriptManager;

    class SceneManager {
    public:
        SceneManager(ResourceManager &resources, SEPostProcess &postProcess,
            ParticleSystem &particleSystem, SEInputSystem &input, ScriptManager& scripts);
        ~SceneManager();

        SceneManager(const SceneManager &) = delete;
        SceneManager &operator=(const SceneManager &) = delete;
        SceneManager(SceneManager &&) = delete;
        SceneManager &operator=(SceneManager &&) = delete;

        void destroyAllScenes();
        Scene& createScene(const std::string& name);
        Scene* getScene(const std::string& name);
        void setActiveScene(const std::string& name);
        Scene* getActiveScene();
        SECamera& getCamera();

        bool saveActiveScene(const std::string& filePath);

        bool saveScene(const std::string& sceneName, const std::string& filePath);

        bool loadScene(const std::string& filePath, const std::string& sceneName);


    private:
        ResourceManager &resourceManager;
        SEPostProcess &postProcess;
        ParticleSystem &particleSystem;
        SEInputSystem &inputSystem;
        ScriptManager &scriptManager;

        std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
        Scene* activeScene = nullptr;

    };

} // namespace se
