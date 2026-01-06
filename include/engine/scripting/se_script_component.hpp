#pragma once

#include <memory>

namespace se {

    class SEGameObject;

    class ScriptComponent {
    public:
        virtual ~ScriptComponent() = default;

        virtual void onCreate() {}
        virtual void onUpdate(float deltaTime) {}
        virtual void onDestroy() {}

        void setOwner(SEGameObject* gameObject) { owner = gameObject; }

        virtual std::string getName() const = 0;

    protected:
        SEGameObject* owner = nullptr;
    };

} // namespace se
