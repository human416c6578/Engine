#include "se_scene_manager.hpp"
#include "se_script_component.hpp"
#include <glm/gtc/type_ptr.hpp>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

bool se::SceneManager::saveScene(const std::string& sceneName, const std::string& filePath) {
    se::Scene* scene = getScene(sceneName);
    if (!scene) return false;

    json jscene;
    jscene["scene"]["name"] = scene->getName();
    
    auto& camera = scene->getCamera();

    // Save translation
    auto transform = camera.getTransform();
    auto pos = camera.getTransform().translation;
    auto rot = camera.getTransform().rotation;

    jscene["scene"]["camera"]["translation"] = { pos.x, pos.y, pos.z };
    jscene["scene"]["camera"]["rotation"] = { rot.x, rot.y, rot.z };

    // Save projection matrix (16 floats)
    const glm::mat4& proj = camera.getProjection();
    jscene["scene"]["camera"]["projection"] = std::vector<float>(glm::value_ptr(proj), glm::value_ptr(proj) + 16);

    // Save view matrix (16 floats)
    const glm::mat4& view = camera.getView();
    jscene["scene"]["camera"]["view"] = std::vector<float>(glm::value_ptr(view), glm::value_ptr(view) + 16);


    std::unordered_set<std::shared_ptr<SEMaterial>> usedMaterials;
    std::unordered_set<std::shared_ptr<SEMesh>> usedMeshes;
    std::unordered_set<std::shared_ptr<SETexture>> usedTextures;

    for (const auto& go : scene->getGameObjects()) {
        json jgo;
        jgo["id"] = go->getId();
        jgo["name"] = go->getName();

        // Transform
        const auto& t = go->getTransform();
        jgo["transform"]["position"] = { t.translation.x, t.translation.y, t.translation.z };
        jgo["transform"]["rotation"] = { t.rotation.x, t.rotation.y, t.rotation.z };
        jgo["transform"]["scale"] = { t.scale.x, t.scale.y, t.scale.z };

        // Mesh
        if (auto mesh = go->getMesh()) {
            jgo["mesh"] = mesh->getGUID();
            usedMeshes.insert(mesh);
        }

        // Material
        if (auto mat = go->getMaterial()) {
            jgo["material"] = mat->getGUID();
            usedMaterials.insert(mat);
        }

        // Light
        if (go->hasLight()) {
            const auto& l = go->getLight();
            jgo["light"]["type"] = l.type;
            jgo["light"]["color"] = { l.color.r, l.color.g, l.color.b };
            jgo["light"]["intensity"] = l.intensity;
        }

        // Script
        if (auto& script = go->getScript()) {
            jgo["script"] = script->getName();
        }

        jscene["scene"]["gameObjects"].push_back(jgo);
    }

    for (const auto& mat : usedMaterials) {
        if (auto tex = mat->getDiffuseTexture()) usedTextures.insert(tex);
        if (auto tex = mat->getNormalTexture()) usedTextures.insert(tex);
        if (auto tex = mat->getMetallicTexture()) usedTextures.insert(tex);
        if (auto tex = mat->getRoughnessTexture()) usedTextures.insert(tex);
        if (auto tex = mat->getAOTexture()) usedTextures.insert(tex);
    }

    json jmaterials = json::array();
    for (const auto& mat : usedMaterials) {
        json jmat;
        jmat["guid"] = mat->getGUID();
        jmat["name"] = mat->getName();
        jmat["color"] = { mat->getColor().r, mat->getColor().g, mat->getColor().b };
        jmat["metallic"] = mat->getMetallic();
        jmat["roughness"] = mat->getRoughness();
        jmat["ao"] = mat->getAO();
        if (auto tex = mat->getDiffuseTexture()) jmat["diffuseTexture"] = tex->getGUID();
        if (auto tex = mat->getNormalTexture()) jmat["normalTexture"] = tex->getGUID();
        if (auto tex = mat->getMetallicTexture()) jmat["metallicTexture"] = tex->getGUID();
        if (auto tex = mat->getRoughnessTexture()) jmat["roughnessTexture"] = tex->getGUID();
        if (auto tex = mat->getAOTexture()) jmat["aoTexture"] = tex->getGUID();
        jmaterials.push_back(jmat);
    }
    jscene["scene"]["materials"] = jmaterials;

    json jmeshes = json::array();
    for (const auto& mesh : usedMeshes) {
        json jmesh;
        jmesh["guid"] = mesh->getGUID();
        jmesh["name"] = mesh->getName();
        jmesh["path"] = resourceManager->getResourcePath(mesh->getGUID());

        jmeshes.push_back(jmesh);
    }
    jscene["scene"]["meshes"] = jmeshes;

    json jtextures = json::array();
    for (const auto& tex : usedTextures) {
        json jtex;
        jtex["guid"] = tex->getGUID();
        jtex["name"] = tex->getName();
        jtex["path"] = resourceManager->getResourcePath(tex->getGUID());

        if(!jtex["path"].empty())
            jtextures.push_back(jtex);
    }
    jscene["scene"]["textures"] = jtextures;

    std::ofstream file(filePath);
    file << jscene.dump(4);
    return true;
}

bool se::SceneManager::saveActiveScene(const std::string& filePath)
{
    return saveScene(activeScene->getName(), filePath);
}

bool se::SceneManager::loadScene(const std::string& filePath, const std::string& sceneName)
{
    std::ifstream file(filePath);
    if (!file) return false;

    json jscene;
    file >> jscene;
    auto& sceneData = jscene["scene"];

    Scene& scene = createScene(sceneName);
    setActiveScene(sceneName);

    auto& camera = scene.getCamera();
    if (jscene["scene"].contains("camera")) {
        auto jcamera = jscene["scene"]["camera"];
        TransformComponent transform;

        // Load translation
        if (jcamera.contains("translation")) {
            auto t = jcamera["translation"];
            glm::vec3 translation(t[0].get<float>(), t[1].get<float>(), t[2].get<float>());
            transform.translation = translation;
            
        }

        // Load rotation
        if (jcamera.contains("rotation")) {
            auto r = jcamera["rotation"];
            glm::vec3 rotation(r[0].get<float>(), r[1].get<float>(), r[2].get<float>());
            transform.rotation = rotation;
        }

        camera.setTransform(transform);

        // Load projection matrix
        if (jcamera.contains("projection")) {
            auto projArray = jcamera["projection"];
            glm::mat4 projMat;
            for (int i = 0; i < 16; ++i) {
                projMat[i / 4][i % 4] = projArray[i].get<float>();
            }
            camera.setProjection(projMat);
        }

        // Load view matrix
        if (jcamera.contains("view")) {
            auto viewArray = jcamera["view"];
            glm::mat4 viewMat;
            for (int i = 0; i < 16; ++i) {
                viewMat[i / 4][i % 4] = viewArray[i].get<float>();
            }
            camera.setView(viewMat);
        }
        camera.setViewYXZ();
    }

    // --- Load meshes ---
    if (sceneData.contains("meshes")) {
        for (const auto& jmesh : sceneData["meshes"]) {
            std::string guid = jmesh["guid"];
            std::string name = jmesh["name"];
            std::string path = jmesh["path"];
            std::shared_ptr<SEMesh> mesh;
            if (path == "CUBE") {
                mesh = resourceManager->createCube(name);
            }
            else if (path == "SPHERE") {
                mesh = resourceManager->createSphere(name);
            }
            else if (!path.empty()) {
                mesh = resourceManager->loadMesh(guid, path);
            }
        }
    }

    // --- Load textures ---
    if (sceneData.contains("textures")) {
        for (const auto& jtex : sceneData["textures"]) {
            std::string guid = jtex["guid"];
            std::string path = jtex["path"];
            if (guid != "GUID_DUMMY") {
                auto tex = resourceManager->loadTexture(guid, path);
            }
        }
    }

    // --- Load materials ---
    if (sceneData.contains("materials")) {
        for (const auto& jmat : sceneData["materials"]) {
            std::string guid = jmat["guid"];
            std::string name = jmat["name"];
            auto mat = resourceManager->getMaterial(guid);
            if (!mat) {
                mat = resourceManager->createMaterial(guid, name);
            }
            // Set properties
            mat->setName(name);
            mat->setColor({ jmat["color"][0], jmat["color"][1], jmat["color"][2] });
            mat->setMetallic(jmat["metallic"]);
            mat->setRoughness(jmat["roughness"]);
            mat->setAO(jmat["ao"]);
            // Set textures by GUID
            if (jmat.contains("diffuseTexture")) {
                std::string texGuid = jmat["diffuseTexture"];
                auto texture = resourceManager->getTexture(texGuid);
                if (texture) mat->setDiffuseTexture(texture);
            }
            if (jmat.contains("normalTexture")) {
                std::string texGuid = jmat["normalTexture"];
                auto texture = resourceManager->getTexture(texGuid);
                if (texture) mat->setNormalTexture(texture);
            }
            if (jmat.contains("metallicTexture")) {
                std::string texGuid = jmat["metallicTexture"];
                auto texture = resourceManager->getTexture(texGuid);
                if (texture) mat->setMetallicTexture(texture);
            }
            if (jmat.contains("roughnessTexture")) {
                std::string texGuid = jmat["roughnessTexture"];
                auto texture = resourceManager->getTexture(texGuid);
                if (texture) mat->setRoughnessTexture(texture);
            }
            if (jmat.contains("aoTexture")) {
                std::string texGuid = jmat["aoTexture"];
                auto texture = resourceManager->getTexture(texGuid);
                if (texture) mat->setAOTexture(texture);
            }
        }
    }

    // --- Create game objects ---
    for (const auto& jgo : sceneData["gameObjects"]) {
        auto& go = scene.createGameObjectRef(jgo["name"]);

        // Transform
        if (jgo.contains("transform")) {
            auto& t = go.getTransform();
            auto pos = jgo["transform"]["position"];
            auto rot = jgo["transform"]["rotation"];
            auto scale = jgo["transform"]["scale"];
            t.translation = { pos[0], pos[1], pos[2] };
            t.rotation = { rot[0], rot[1], rot[2] };
            t.scale = { scale[0], scale[1], scale[2] };
        }

        // Mesh
        if (jgo.contains("mesh")) {
            std::string meshGuid = jgo["mesh"];
            auto mesh = resourceManager->getMesh(meshGuid);
            if (mesh) go.setMesh(mesh);
        }

        // Material
        if (jgo.contains("material")) {
            std::string matGuid = jgo["material"];
            auto mat = resourceManager->getMaterial(matGuid);
            if (mat) go.setMaterial(mat);
        }

        // Light
        if (jgo.contains("light")) {
            auto& l = go.getLight();
            auto& jlight = jgo["light"];
            l.type = static_cast<LightType>(jlight["type"].get<int>());
            auto& color = jlight["color"];
            l.color = { color[0], color[1], color[2] };
            l.intensity = jlight["intensity"];
            go.setLight(l);
        }

        // Script
        if (jgo.contains("script")) {
            std::string scriptName = jgo["script"];
            auto script = createScript(scriptName);

            if(script)
                go.setScript(std::move(script));
        }
    }

    return true;
}

