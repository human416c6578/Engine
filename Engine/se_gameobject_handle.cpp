#include "se_gameobject_handle.hpp"
#include "se_scene.hpp"

namespace se {
    SEGameObject* GameObjectHandle::get() const {
        if (scene)
            return scene->getGameObjectById(id);
        return nullptr;
    }

    void GameObjectHandle::destroy() const {
        if (!scene) {
            std::cerr << "[GameObjectHandle] Tried to destroy object, but scene is null!\n";
            return;
        }
        scene->destroyGameObject(id);
    }

}
