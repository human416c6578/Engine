#include "engine/scene/se_gameobject_handle.hpp"
#include "engine/core/se_logger.hpp"
#include "engine/scene/se_scene.hpp"

namespace se {
    SEGameObject* GameObjectHandle::get() const {
        if (scene)
            return scene->getGameObjectById(id);
        return nullptr;
    }

    void GameObjectHandle::destroy() const {
        if (!scene) {
            log::warn("GameObjectHandle", "Tried to destroy object, but scene is null!");
            return;
        }
        scene->destroyGameObject(id);
    }

}
