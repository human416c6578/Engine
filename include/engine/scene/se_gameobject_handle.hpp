#pragma once
#include <stdexcept>

namespace se {
    class Scene;
    class SEGameObject;

    class GameObjectHandle {
        int id;
        Scene* scene;

    public:
        GameObjectHandle() : id(-1), scene(nullptr) {}
        GameObjectHandle(int id, Scene* scene) : id(id), scene(scene) {}

        SEGameObject* get() const;
        void destroy() const;

        SEGameObject& operator*() const {
            auto ptr = get();
            if (!ptr) throw std::runtime_error("Invalid GameObjectHandle");
            return *ptr;
        }

        SEGameObject* operator->() const {
            return get();
        }

        explicit operator bool() const {
            return get() != nullptr;
        }

        int getId() const { return id; }
    };
}
