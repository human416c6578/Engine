#pragma once

#include <memory>
#include <string>

namespace se {

    class Logger;
    class SEGameObject;
    class Scene;
    class ResourceManager;
    class SEInputSystem;

    struct ScriptContext
    {
        Scene &scene;
        ResourceManager &resources;
        SEInputSystem &input;
        Logger& logger;
    };

    class ScriptComponent {
    public:
        virtual ~ScriptComponent() = default;

        virtual void onCreate(ScriptContext &) {}
        virtual void onUpdate(float deltaTime, ScriptContext &) {}
        virtual void onDestroy() {}

        void setOwner(SEGameObject* gameObject) { owner = gameObject; }

        virtual std::string getName() const = 0;

    protected:
        SEGameObject* owner = nullptr;
    };

} // namespace se
