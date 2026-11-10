#include "engine/scene/se_scene_manager.hpp"
#include "engine/renderer/se_pbr_material.hpp"
#include "engine/renderer/se_texture.hpp"
#include "engine/scene/se_scene.hpp"
#include "engine/renderer/se_mesh.hpp"
#include "engine/core/se_camera.hpp"
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include "engine/platform/se_input_system.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/renderer/se_postprocess.hpp"
#include "engine/renderer/se_particle_system.hpp"
#include "engine/scripting/se_script_component.hpp"
#include "engine/scripting/se_script_manager.hpp"
#include <glm/gtc/type_ptr.hpp>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

se::SceneManager::SceneManager(ResourceManager &resources, SEPostProcess &postProcess,
                               ParticleSystem &particleSystem, SEInputSystem &input,
                               ScriptManager& scripts)
    : resourceManager{resources}, postProcess{postProcess},
      particleSystem{particleSystem}, inputSystem{input}, scriptManager{scripts} {}

se::SceneManager::~SceneManager() = default;

void se::SceneManager::destroyAllScenes()
{
  activeScene = nullptr;
  scenes.clear();
}

se::Scene& se::SceneManager::createScene(const std::string& name)
{
  auto scene = std::make_unique<Scene>(name);
  Scene& ref = *scene;
  scenes[name] = std::move(scene);
  return ref;
}

se::Scene* se::SceneManager::getScene(const std::string& name)
{
  const auto it = scenes.find(name);
  return it != scenes.end() ? it->second.get() : nullptr;
}

void se::SceneManager::setActiveScene(const std::string& name)
{
  activeScene = getScene(name);
}

se::Scene* se::SceneManager::getActiveScene()
{
  return activeScene;
}

se::SECamera& se::SceneManager::getCamera()
{
  return activeScene->getCamera();
}

bool se::SceneManager::saveScene(const std::string &sceneName,
                                 const std::string &filePath) {
  se::Scene *scene = getScene(sceneName);
  if (!scene)
    return false;

  json jscene;
  jscene["scene"]["name"] = scene->getName();

  auto &camera = scene->getCamera();

  // Save translation
  auto transform = camera.getTransform();
  auto pos = camera.getTransform().translation;
  auto rot = camera.getTransform().rotation;

  jscene["scene"]["camera"]["translation"] = {pos.x, pos.y, pos.z};
  jscene["scene"]["camera"]["rotation"] = {rot.x, rot.y, rot.z};

  // Save projection matrix (16 floats)
  const glm::mat4 &proj = camera.getProjection();
  jscene["scene"]["camera"]["projection"] =
      std::vector<float>(glm::value_ptr(proj), glm::value_ptr(proj) + 16);

  // Save view matrix (16 floats)
  const glm::mat4 &view = camera.getView();
  jscene["scene"]["camera"]["view"] =
      std::vector<float>(glm::value_ptr(view), glm::value_ptr(view) + 16);

  {
    const auto &pp = postProcess.getSettings();
    json jpp;
    jpp["focusDistance"] = pp.focusDistance;
    jpp["focusRange"] = pp.focusRange;
    jpp["blurScale"] = pp.blurScale;
    jpp["maxBlur"] = pp.maxBlur;
    jpp["enableFog"] = pp.enableFog;
    jpp["fogStart"] = pp.fogStart;
    jpp["fogEnd"] = pp.fogEnd;
    jpp["fogIntensity"] = pp.fogIntensity;
    jpp["fogColor"] = {pp.fogColor[0], pp.fogColor[1], pp.fogColor[2]};
    jpp["enableTonemap"] = pp.enableTonemap;
    jpp["exposure"] = pp.exposure;
    jscene["scene"]["postProcess"] = jpp;
  }

  std::unordered_set<std::shared_ptr<SEMaterial>> usedMaterials;
  std::unordered_set<std::shared_ptr<SEMesh>> usedMeshes;
  std::unordered_set<std::shared_ptr<SETexture>> usedTextures;

  for (const auto &go : scene->getGameObjects()) {
    json jgo;
    jgo["id"] = go->getId();
    jgo["name"] = go->getName();

    // Transform
    const auto &t = go->getTransform();
    jgo["transform"]["position"] = {t.translation.x, t.translation.y,
                                    t.translation.z};
    jgo["transform"]["rotation"] = {t.rotation.x, t.rotation.y, t.rotation.z};
    jgo["transform"]["scale"] = {t.scale.x, t.scale.y, t.scale.z};

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
      const auto &l = go->getLight();
      jgo["light"]["type"] = l.type;
      jgo["light"]["color"] = {l.color.r, l.color.g, l.color.b};
      jgo["light"]["intensity"] = l.intensity;
    }

    // Optional physics components; dimensions are local collider dimensions.
    if (const auto* collider = go->getCollider()) {
      auto& jc = jgo["collider"];
      jc["localOffset"] = {collider->localOffset.x, collider->localOffset.y,
                           collider->localOffset.z};
      jc["friction"] = collider->friction;
      if (const auto* box = std::get_if<BoxShape>(&collider->shape)) {
        jc["shape"] = "box";
        jc["halfExtents"] = {box->halfExtents.x, box->halfExtents.y,
                             box->halfExtents.z};
      } else if (const auto* sphere = std::get_if<SphereShape>(&collider->shape)) {
        jc["shape"] = "sphere";
        jc["radius"] = sphere->radius;
      } else if (const auto* cylinder = std::get_if<CylinderShape>(&collider->shape)) {
        jc["shape"] = "cylinder";
        jc["radius"] = cylinder->radius;
        jc["halfHeight"] = cylinder->halfHeight;
      }
    }

    if (const auto* body = go->getRigidbody()) {
      auto& jr = jgo["rigidbody"];
      jr["isStatic"] = body->isStatic;
      jr["useGravity"] = body->useGravity;
      jr["mass"] = body->mass;
      jr["velocity"] = {body->velocity.x, body->velocity.y, body->velocity.z};
    }

    // Script
    if (auto &script = go->getScript()) {
      jgo["script"] = script->getName();
    }

    if (go->hasParticleEmitter()) {
      auto &comp = go->getParticleEmitter();
      if (comp.isValid()) {
        auto &em = particleSystem.getEmitter(comp.handle);

        json jpe;
        json je;
        je["id"] = em.id;
        je["active"] = em.active;

        // SpawnVolume
        json jvol;
        jvol["position"] = {em.volume.position.x, em.volume.position.y,
                            em.volume.position.z, em.volume.position.w};
        jvol["extents"] = {em.volume.extents.x, em.volume.extents.y,
                           em.volume.extents.z, em.volume.extents.w};
        jvol["direction"] = {em.volume.direction.x, em.volume.direction.y,
                             em.volume.direction.z, em.volume.direction.w};
        jvol["type"] = static_cast<uint32_t>(em.volume.type);
        je["volume"] = jvol;

        je["direction"] = {em.direction.x, em.direction.y, em.direction.z,
                           em.direction.w};
        je["speedMin"] = em.speedMin;
        je["speedMax"] = em.speedMax;
        je["lifetimeMin"] = em.lifetimeMin;
        je["lifetimeMax"] = em.lifetimeMax;
        je["colorStart"] = {em.colorStart.x, em.colorStart.y, em.colorStart.z,
                            em.colorStart.w};
        je["colorEnd"] = {em.colorEnd.x, em.colorEnd.y, em.colorEnd.z,
                          em.colorEnd.w};
        je["sizeStart"] = em.sizeStart;
        je["sizeEnd"] = em.sizeEnd;
        je["emitRate"] = em.emitRate;

        // Forces (resolved from handles, not raw indices)
        json jforces = json::array();
        for (uint32_t i = 0; i < em.forceCount; ++i) {
          uint32_t fh = em.forceIndices[i];
          if (fh == ~0u)
            continue;
          try {
            auto &f = particleSystem.getForce(fh);
            json jf;
            jf["position"] = {f.position.x, f.position.y, f.position.z,
                              f.position.w};
            jf["direction"] = {f.direction.x, f.direction.y, f.direction.z,
                               f.direction.w};
            jf["strength"] = f.strength;
            jf["radius"] = f.radius;
            jf["falloff"] = f.falloff;
            jf["type"] = static_cast<uint32_t>(f.type);
            jf["active"] = f.active;
            jforces.push_back(jf);
          } catch (...) { /* stale handle, ignore */
          }
        }

        jpe["emitter"] = je;
        jpe["forces"] = jforces;
        jgo["particleEmitter"] = jpe;
      }
    }

    jscene["scene"]["gameObjects"].push_back(jgo);
  }

  for (const auto &mat : usedMaterials) {
    if (auto tex = mat->getDiffuseTexture())
      usedTextures.insert(tex);
    if (auto tex = mat->getNormalTexture())
      usedTextures.insert(tex);
    if (auto tex = mat->getMetallicTexture())
      usedTextures.insert(tex);
    if (auto tex = mat->getRoughnessTexture())
      usedTextures.insert(tex);
    if (auto tex = mat->getAOTexture())
      usedTextures.insert(tex);
    if (auto tex = mat->getHeightTexture())
      usedTextures.insert(tex);
  }

  json jmaterials = json::array();
  for (const auto &mat : usedMaterials) {
    json jmat;
    jmat["guid"] = mat->getGUID();
    jmat["name"] = mat->getName();
    jmat["color"] = {mat->getColor().r, mat->getColor().g, mat->getColor().b};
    jmat["metallic"] = mat->getMetallic();
    jmat["roughness"] = mat->getRoughness();
    jmat["ao"] = mat->getAO();
    jmat["heightScale"] = mat->getHeightScale();
    jmat["uvScale"] = {mat->getUVScale().x, mat->getUVScale().y};
    if (auto tex = mat->getDiffuseTexture())
      jmat["diffuseTexture"] = tex->getGUID();
    if (auto tex = mat->getNormalTexture())
      jmat["normalTexture"] = tex->getGUID();
    if (auto tex = mat->getMetallicTexture())
      jmat["metallicTexture"] = tex->getGUID();
    if (auto tex = mat->getRoughnessTexture())
      jmat["roughnessTexture"] = tex->getGUID();
    if (auto tex = mat->getAOTexture())
      jmat["aoTexture"] = tex->getGUID();
    if (auto tex = mat->getHeightTexture())
      jmat["heightTexture"] = tex->getGUID();
    jmaterials.push_back(jmat);
  }
  jscene["scene"]["materials"] = jmaterials;

  json jmeshes = json::array();
  for (const auto &mesh : usedMeshes) {
    json jmesh;
    jmesh["guid"] = mesh->getGUID();
    jmesh["name"] = mesh->getName();
    jmesh["path"] = resourceManager.getResourcePath(mesh->getGUID());

    jmeshes.push_back(jmesh);
  }
  jscene["scene"]["meshes"] = jmeshes;

  json jtextures = json::array();
  for (const auto &tex : usedTextures) {
    json jtex;
    jtex["guid"] = tex->getGUID();
    jtex["name"] = tex->getName();
    jtex["path"] = resourceManager.getResourcePath(tex->getGUID());

    if (!jtex["path"].empty())
      jtextures.push_back(jtex);
  }
  jscene["scene"]["textures"] = jtextures;

  std::ofstream file(filePath);
  file << jscene.dump(4);
  return true;
}

bool se::SceneManager::saveActiveScene(const std::string &filePath) {
  return saveScene(activeScene->getName(), filePath);
}

bool se::SceneManager::loadScene(const std::string &filePath,
                                 const std::string &sceneName) {
  std::ifstream file(filePath);
  if (!file)
    return false;

  json jscene;
  file >> jscene;
  auto &sceneData = jscene["scene"];

  Scene &scene = createScene(sceneName);
  setActiveScene(sceneName);

  auto &camera = scene.getCamera();
  if (jscene["scene"].contains("camera")) {
    auto jcamera = jscene["scene"]["camera"];
    TransformComponent transform;

    // Load translation
    if (jcamera.contains("translation")) {
      auto t = jcamera["translation"];
      glm::vec3 translation(t[0].get<float>(), t[1].get<float>(),
                            t[2].get<float>());
      transform.translation = translation;
    }

    // Load rotation
    if (jcamera.contains("rotation")) {
      auto r = jcamera["rotation"];
      glm::vec3 rotation(r[0].get<float>(), r[1].get<float>(),
                         r[2].get<float>());
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

  if (jscene["scene"].contains("postProcess")) {
    auto &jpp = jscene["scene"]["postProcess"];
    se::SEPostProcess::PostProcessSettings pp;
    pp.focusDistance = jpp.value("focusDistance", 0.1f);
    pp.focusRange = jpp.value("focusRange", 1.1f);
    pp.blurScale = jpp.value("blurScale", 2.0f);
    pp.maxBlur = jpp.value("maxBlur", 1.0f);
    pp.enableFog = jpp.value("enableFog", 1);
    pp.fogStart = jpp.value("fogStart", 0.0f);
    pp.fogEnd = jpp.value("fogEnd", 30.0f);
    pp.fogIntensity = jpp.value("fogIntensity", 0.15f);
    if (jpp.contains("fogColor") && jpp["fogColor"].is_array() &&
        jpp["fogColor"].size() >= 3) {
      pp.fogColor[0] = jpp["fogColor"][0];
      pp.fogColor[1] = jpp["fogColor"][1];
      pp.fogColor[2] = jpp["fogColor"][2];
    }
    pp.enableTonemap = jpp.value("enableTonemap", 1);
    pp.exposure = jpp.value("exposure", 1.0f);
    postProcess.updateSettings(pp);
  }

  // --- Load meshes ---
  if (sceneData.contains("meshes")) {
    for (const auto &jmesh : sceneData["meshes"]) {
      std::string guid = jmesh["guid"];
      std::string name = jmesh["name"];
      std::string path = jmesh["path"];
      std::shared_ptr<SEMesh> mesh;
      if (path == "CUBE") {
        mesh = resourceManager.createCube(name);
      } else if (path == "SPHERE") {
        mesh = resourceManager.createSphere(name);
      } else if (path == "CYLINDER") {
        mesh = resourceManager.createCylinder(name);
      } else if (path == "PLANE") {
        mesh = resourceManager.createPlane(name);
      } else if (!path.empty()) {
        mesh = resourceManager.loadMesh(guid, path);
      }
    }
  }

  // --- Load textures ---
  if (sceneData.contains("textures")) {
    for (const auto &jtex : sceneData["textures"]) {
      std::string guid = jtex["guid"];
      std::string path = jtex["path"];
      if (guid != "GUID_DUMMY") {
        auto tex = resourceManager.loadTexture(guid, path);
      }
    }
  }

  // --- Load materials ---
  if (sceneData.contains("materials")) {
    for (const auto &jmat : sceneData["materials"]) {
      std::string guid = jmat["guid"];
      std::string name = jmat["name"];
      auto mat = resourceManager.getMaterial(guid);
      if (!mat) {
        mat = resourceManager.createMaterial(guid, name);
      }
      // Set properties
      mat->setName(name);
      mat->setColor({jmat["color"][0], jmat["color"][1], jmat["color"][2]});
      mat->setMetallic(jmat["metallic"]);
      mat->setRoughness(jmat["roughness"]);
      mat->setAO(jmat["ao"]);
      if (jmat.contains("heightScale"))
        mat->setHeightScale(jmat["heightScale"]);
      if(jmat.contains("uvScale"))
        mat->setUVScale({jmat["uvScale"][0],jmat["uvScale"][1]});
      // Set textures by GUID
      if (jmat.contains("diffuseTexture")) {
        std::string texGuid = jmat["diffuseTexture"];
        auto texture = resourceManager.getTexture(texGuid);
        if (texture)
          mat->setDiffuseTexture(texture);
      }
      if (jmat.contains("normalTexture")) {
        std::string texGuid = jmat["normalTexture"];
        auto texture = resourceManager.getTexture(texGuid);
        if (texture)
          mat->setNormalTexture(texture);
      }
      if (jmat.contains("metallicTexture")) {
        std::string texGuid = jmat["metallicTexture"];
        auto texture = resourceManager.getTexture(texGuid);
        if (texture)
          mat->setMetallicTexture(texture);
      }
      if (jmat.contains("roughnessTexture")) {
        std::string texGuid = jmat["roughnessTexture"];
        auto texture = resourceManager.getTexture(texGuid);
        if (texture)
          mat->setRoughnessTexture(texture);
      }
      if (jmat.contains("aoTexture")) {
        std::string texGuid = jmat["aoTexture"];
        auto texture = resourceManager.getTexture(texGuid);
        if (texture)
          mat->setAOTexture(texture);
      }
      if (jmat.contains("heightTexture")) {
        std::string texGuid = jmat["heightTexture"];
        auto texture = resourceManager.getTexture(texGuid);
        if (texture)
          mat->setHeightTexture(texture);
      }
    }
  }

  // --- Create game objects ---
  for (const auto &jgo : sceneData["gameObjects"]) {
    auto &go = scene.createGameObjectRef(jgo["name"]);

    // Transform
    if (jgo.contains("transform")) {
      auto &t = go.getTransform();
      auto pos = jgo["transform"]["position"];
      auto rot = jgo["transform"]["rotation"];
      auto scale = jgo["transform"]["scale"];
      t.translation = {pos[0], pos[1], pos[2]};
      t.rotation = {rot[0], rot[1], rot[2]};
      t.scale = {scale[0], scale[1], scale[2]};
    }

    // Mesh
    if (jgo.contains("mesh")) {
      std::string meshGuid = jgo["mesh"];
      auto mesh = resourceManager.getMesh(meshGuid);
      if (mesh)
        go.setMesh(mesh);
    }

    // Material
    if (jgo.contains("material")) {
      std::string matGuid = jgo["material"];
      auto mat = resourceManager.getMaterial(matGuid);
      if (mat)
        go.setMaterial(mat);
    }

    // Light
    if (jgo.contains("light")) {
      auto &l = go.getLight();
      auto &jlight = jgo["light"];
      l.type = static_cast<LightType>(jlight["type"].get<int>());
      auto &color = jlight["color"];
      l.color = {color[0], color[1], color[2]};
      l.intensity = jlight["intensity"];
      go.setLight(l);
    }

    // Load physics before script creation so onCreate can inspect the components.
    try {
      const auto readVec3 = [](const json& values) -> glm::vec3 {
        if (!values.is_array() || values.size() != 3)
          throw std::invalid_argument("physics vector must contain three numbers");
        const glm::vec3 result{values.at(0).get<float>(), values.at(1).get<float>(),
                               values.at(2).get<float>()};
        if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z))
          throw std::invalid_argument("physics vector must be finite");
        return result;
      };

      if (jgo.contains("collider")) {
        const auto& jc = jgo.at("collider");
        SECollider collider{};
        const auto shape = jc.value("shape", std::string{"box"});
        if (shape == "box") {
          BoxShape box{};
          if (jc.contains("halfExtents")) box.halfExtents = readVec3(jc.at("halfExtents"));
          if (box.halfExtents.x <= 0.0f || box.halfExtents.y <= 0.0f || box.halfExtents.z <= 0.0f)
            throw std::invalid_argument("box half-extents must be positive");
          collider.shape = box;
        } else if (shape == "sphere") {
          SphereShape sphere{};
          sphere.radius = jc.value("radius", sphere.radius);
          if (!std::isfinite(sphere.radius) || sphere.radius <= 0.0f)
            throw std::invalid_argument("sphere radius must be finite and positive");
          collider.shape = sphere;
        } else if (shape == "cylinder") {
          CylinderShape cylinder{};
          cylinder.radius = jc.value("radius", cylinder.radius);
          cylinder.halfHeight = jc.value("halfHeight", cylinder.halfHeight);
          if (!std::isfinite(cylinder.radius) || cylinder.radius <= 0.0f ||
              !std::isfinite(cylinder.halfHeight) || cylinder.halfHeight <= 0.0f)
            throw std::invalid_argument("cylinder radius and half-height must be finite and positive");
          collider.shape = cylinder;
        } else {
          throw std::invalid_argument("unknown collider shape");
        }
        if (jc.contains("localOffset")) collider.localOffset = readVec3(jc.at("localOffset"));
        collider.friction = jc.value("friction", collider.friction);
        if (!std::isfinite(collider.friction) || collider.friction < 0.0f)
          throw std::invalid_argument("friction must be finite and nonnegative");
        go.setCollider(collider);
      }

      if (jgo.contains("rigidbody")) {
        const auto& jr = jgo.at("rigidbody");
        SERigidbody body{};
        body.isStatic = jr.value("isStatic", body.isStatic);
        body.useGravity = jr.value("useGravity", body.useGravity);
        body.mass = jr.value("mass", body.mass);
        if (!std::isfinite(body.mass) || body.mass <= 0.0f)
          throw std::invalid_argument("rigidbody mass must be finite and positive");
        if (jr.contains("velocity")) body.velocity = readVec3(jr.at("velocity"));
        go.setRigidbody(body);
      }
    } catch (const std::exception& error) {
      log::error("Scene", "Invalid physics component on '%s': %s",
                 go.getName().c_str(), error.what());
      return false;
    }

    // Script
    if (jgo.contains("script")) {
      std::string scriptName = jgo["script"];
      auto script = scriptManager.createScript(scriptName);

      if (script)
      {
        ScriptContext context = scene.scriptContext(resourceManager, inputSystem);
        go.setScript(std::move(script), context);
      }
    }

    if (jgo.contains("particleEmitter")) {
      auto &jpe = jgo["particleEmitter"];

      se::Emitter em{};
      if (jpe.contains("emitter")) {
        auto &je = jpe["emitter"];
        em.id = je.value("id", 0u);
        em.active = je.value("active", 1);

        if (je.contains("volume")) {
          auto &jv = je["volume"];
          auto pos = jv["position"];
          em.volume.position = {pos[0], pos[1], pos[2], pos[3]};
          auto ext = jv["extents"];
          em.volume.extents = {ext[0], ext[1], ext[2], ext[3]};
          auto dir = jv["direction"];
          em.volume.direction = {dir[0], dir[1], dir[2], dir[3]};
          em.volume.type = static_cast<se::VolumeType>(jv.value("type", 0u));
        }

        if (je.contains("direction")) {
          auto jd = je["direction"];
          em.direction = {jd[0], jd[1], jd[2], jd[3]};
        }

        em.speedMin = je.value("speedMin", 1.0f);
        em.speedMax = je.value("speedMax", 3.0f);
        em.lifetimeMin = je.value("lifetimeMin", 1.0f);
        em.lifetimeMax = je.value("lifetimeMax", 3.0f);

        if (je.contains("colorStart")) {
          auto c = je["colorStart"];
          em.colorStart = {c[0], c[1], c[2], c[3]};
        }
        if (je.contains("colorEnd")) {
          auto c = je["colorEnd"];
          em.colorEnd = {c[0], c[1], c[2], c[3]};
        }

        em.sizeStart = je.value("sizeStart", 4.0f);
        em.sizeEnd = je.value("sizeEnd", 1.0f);
        em.emitRate = je.value("emitRate", 100.0f);
      }

      auto ehandle = particleSystem.addEmitter(em);

      if (jpe.contains("forces")) {
        for (auto &jf : jpe["forces"]) {
          se::Force f{};
          if (jf.contains("position")) {
            auto p = jf["position"];
            f.position = {p[0], p[1], p[2], p[3]};
          }
          if (jf.contains("direction")) {
            auto d = jf["direction"];
            f.direction = {d[0], d[1], d[2], d[3]};
          }
          f.strength = jf.value("strength", 1.0f);
          f.radius = jf.value("radius", 0.0f);
          f.falloff = jf.value("falloff", 0.0f);
          f.type = static_cast<se::ForceType>(jf.value("type", 0u));
          f.active = jf.value("active", 1u);

          auto fhandle = particleSystem.addForce(f);
          particleSystem.attachForce(ehandle, fhandle);
        }
      }

      go.getParticleEmitter() = {ehandle, &particleSystem};
    }
  }

  return true;
}
