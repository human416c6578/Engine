#include "engine/editor/imgui_manager.hpp"
#include "engine/core/se_performance.hpp"
#include "engine/vulkan/se_device.hpp"
#include "engine/renderer/se_particle_system.hpp"
#include "engine/renderer/se_pbr.hpp"
#include "engine/renderer/se_postprocess.hpp"
#include "engine/renderer/se_material_system.hpp"
#include "engine/renderer/se_mesh_system.hpp"
#include "engine/renderer/se_texture_system.hpp"
#include "engine/renderer/se_pbr_material.hpp"
#include "engine/renderer/se_mesh.hpp"
#include "engine/renderer/se_texture.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/scene/se_scene_manager.hpp"
#include "engine/scene/se_scene.hpp"
#include "engine/scene/se_gameobject.hpp"
#include "engine/scripting/se_script_component.hpp"
#include "engine/scripting/se_script_manager.hpp"
#include "engine/platform/se_input_system.hpp"
#include "engine/core/se_frame_config.hpp"
#include "engine/core/se_logger.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_filedialog/ImGuiFileDialog.h>

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <exception>

namespace
{
    void checkImGuiVulkanResult(VkResult result) noexcept
    {
        if (result >= VK_SUCCESS)
        {
            return;
        }

        // Backend temporaries are not exception-safe, and this callback cannot
        // distinguish creation errors from unknown upload completion. Never unwind.
        se::log::error("Vulkan", "ImGui Vulkan backend operation failed with VkResult %d; terminating without unwinding",
            static_cast<int>(result));
        std::terminate();
    }
}

void se::ImGuiManager::renderSceneHierarchy()
{
    bool itemHovered = false;
    auto scene = sceneManager.getActiveScene();
    std::list<std::unique_ptr<SEGameObject>>& gameobjects = scene->getGameObjects();

    if (!showSceneHierarchy) return;

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    // Fixed to left, above Asset Browser, 300px wide
    float assetBrowserHeight = 200.0f;
    ImGui::SetNextWindowPos(workPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, workSize.y - assetBrowserHeight), ImGuiCond_Always);

    ImGui::Begin("Scene Hierarchy", &showSceneHierarchy,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Game Objects (%zu)", gameobjects.size());
    ImGui::Separator();

    for (size_t i = 0; i < gameobjects.size(); i++)
    {
        auto it = gameobjects.begin();
        std::advance(it, i);
        auto& gameObject = *it;
        ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
        bool isSelected = (selectedGameObjectIndex == static_cast<int>(i));

        if (ImGui::Selectable((gameObject->getName() + "##" +std::to_string(gameObject->getId())).c_str(), isSelected, flags))
        {
            selectedGameObjectIndex = i;
            selectedAssetName = "";
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            itemHovered = true;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                auto it = gameobjects.begin();
                std::advance(it, i);
                if (it->get()->getParticleEmitter().isValid())
                {
                    particleSystem.removeEmitter(it->get()->getParticleEmitter().handle);
                }
                gameobjects.erase(it);
                if (selectedGameObjectIndex == i) selectedGameObjectIndex = -1;
                else if (selectedGameObjectIndex > i) selectedGameObjectIndex--;
            }
            ImGui::EndPopup();
        }
    }

    if (!itemHovered && ImGui::BeginPopupContextWindow()) {
        if (ImGui::MenuItem("Create Empty")) {
            scene->createGameObjectRef("Empty");

        }
        if (ImGui::MenuItem("Create Plane")) {
            auto& go = scene->createGameObjectRef("Plane");
            go.setMesh(resourceManager.createPlane("PlaneMesh_" + std::to_string(++cubeCount)));
            go.setMaterial(resourceManager.createMaterial("PlaneMaterial_" + std::to_string(cubeCount)));
        }
        if (ImGui::MenuItem("Create Cube")) {
            auto& go = scene->createGameObjectRef("Cube");
            go.setMesh(resourceManager.createCube("CubeMesh_" + std::to_string(++cubeCount)));
            go.setMaterial(resourceManager.createMaterial("CubeMaterial_" + std::to_string(cubeCount)));
        }
        if (ImGui::MenuItem("Create Sphere")) {
            auto& go = scene->createGameObjectRef("Sphere");
            go.setMesh(resourceManager.createSphere("SphereMesh_" + std::to_string(++sphereCount)));
            go.setMaterial(resourceManager.createMaterial("SphereMaterial_" + std::to_string(sphereCount)));

        }
        if (ImGui::MenuItem("Create Cylinder")) {
            auto& go = scene->createGameObjectRef("Cylinder");
            go.setMesh(resourceManager.createCylinder("CylinderMesh"));
            go.setMaterial(resourceManager.createMaterial("CylinderMaterial_" + std::to_string(go.getId())));
        }
        if (ImGui::MenuItem("Create Light")) {
            auto& go = scene->createGameObjectRef("Light");
            Light light{};
			light.type = LightType::Point;
			light.color = { 1.0f, 1.0f, 1.0f };
			light.intensity = 1.0f;
            go.setLight(light);
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}

void se::ImGuiManager::renderAssetBrowser()
{
    if (!showAssetBrowser) return;

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    // Fixed to bottom left, 300px wide, 200px tall
    ImGui::SetNextWindowPos(ImVec2(workPos.x, workPos.y + workSize.y - 200), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);

    ImGui::Begin("Asset Browser", &showAssetBrowser,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Asset Categories");
    ImGui::Separator();

    for (const auto& category : assetCategories)
    {
        ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
        bool isSelected = (selectedAssetCategory == category);

        if (ImGui::Selectable(category.c_str(), isSelected, flags))
        {
            selectedAssetCategory = category;
           /* selectedAssetName = "";
            selectedGameObjectIndex = -1;*/
        }
    }

    renderAssetContextMenu();

    ImGui::End();
}

void se::ImGuiManager::renderAssetContextMenu()
{
    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::MenuItem("Create Material")) {
            std::string name = "New Material " + std::to_string(resourceManager.getMaterials()->size());
            resourceManager.createMaterial(name);
        }
        if (ImGui::MenuItem("Load Mesh"))
        {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 0;
            config.flags = ImGuiFileDialogFlags_CaseInsensitiveExtentionFiltering;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModel", "Select Model Files", "Model Files{.obj,.fbx}", config);

        }
        if (ImGui::MenuItem("Load Texture")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 0;
            config.flags = ImGuiFileDialogFlags_CaseInsensitiveExtentionFiltering;

            ImGuiFileDialog::Instance()->OpenDialog("ChooseTexture", "Select Texture Files", "Texture Files{.png,.jpg,.jpeg,.tga,.bmp,.hdr}", config);

        }
        if (ImGui::MenuItem("Load Scene"))
        {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 0;
            config.flags = ImGuiFileDialogFlags_CaseInsensitiveExtentionFiltering;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseScene", "Select Scene File", "Scene File{.json}", config);

        }
        if (ImGui::MenuItem("Save Scene")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
            ImGuiFileDialog::Instance()->OpenDialog("SaveScene", "Save Scene File", "Scene File{.json}", config);
        }
        ImGui::EndPopup();
    }



    if (ImGuiFileDialog::Instance()->Display("ChooseModel", ImGuiWindowFlags_NoCollapse, ImVec2(400.0f, 200.0f))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

            resourceManager.loadMesh(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseTexture", ImGuiWindowFlags_NoCollapse, ImVec2(400.0f, 200.0f))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            auto selection = ImGuiFileDialog::Instance()->GetSelection();

            for (const auto& [filename, filepathname] : selection)
            {

                resourceManager.loadTexture(filepathname);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseScene", ImGuiWindowFlags_NoCollapse, ImVec2(400.0f, 200.0f))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            auto selection = ImGuiFileDialog::Instance()->GetSelection();
            if (!selection.empty()) {
                const auto& [filename, filepathname] = *selection.begin();
                sceneManager.loadScene(filepathname, filename);
                sceneManager.setActiveScene(filename);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("SaveScene", ImGuiWindowFlags_NoCollapse, ImVec2(400.0f, 200.0f))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            sceneManager.saveActiveScene(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void se::ImGuiManager::renderAssetViewer()
{
    if (!showAssetViewer) return;

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    float left = workPos.x + 300.0f;
    float right = workPos.x + workSize.x - 300.0f;
    ImVec2 viewerPos(left, workPos.y + workSize.y - 200.0f);
    ImVec2 viewerSize(right - left, 200.0f);

    ImGui::SetNextWindowPos(viewerPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(viewerSize, ImGuiCond_Always);

    ImGui::Begin("Asset Viewer", &showAssetViewer,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Category: %s", selectedAssetCategory.c_str());
    ImGui::Separator();

    if (selectedAssetCategory == "Meshes")
    {
        auto meshes = resourceManager.getMeshes();
        for (const auto& [key, mesh] : *meshes)
        {
            std::string name = mesh->getName().empty() ? key : mesh->getName();

            ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
            bool isSelected = (selectedAssetName == key);

            if (ImGui::Selectable(name.c_str(), isSelected, flags))
            {
                selectedAssetName = key;
                selectedGameObjectIndex = -1;
            }
        }
    }
    else if (selectedAssetCategory == "Materials")
    {
        auto materials = resourceManager.getMaterials();
        for (const auto& [key, material] : *materials)
        {
            std::string name = material->getName().empty() ? key : material->getName();
            ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
            bool isSelected = (selectedAssetName == key);

            if (ImGui::Selectable(name.c_str(), isSelected, flags))
            {
                selectedAssetName = key;
                selectedGameObjectIndex = -1;
            }
        }
    }
    else if (selectedAssetCategory == "Textures")
    {
        ImGui::InputTextWithHint("##Search", "Search textures...",
            assetBrowserTextureSearch.data(), assetBrowserTextureSearch.size());

        ImGui::Separator();

        const float thumbnailSize = 64.0f;
        const float padding = 8.0f;
        float cellSize = thumbnailSize + padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        ImGui::Columns(columnCount, nullptr, false);

        auto textures = resourceManager.getTextures();
        for (const auto& [key, texture] : *textures)
        {
            const std::string& name = texture->getName().empty() ? key : texture->getName();
            if (assetBrowserTextureSearch[0] != '\0' &&
                name.find(assetBrowserTextureSearch.data()) == std::string::npos)
                continue;

            ImGui::PushID(texture.get());

            ImVec2 cursor = ImGui::GetCursorScreenPos();

            // Draw the texture thumbnail
            ImGui::Image((ImTextureID)texture->getTextureDescriptorSet(), { thumbnailSize, thumbnailSize });

            // Draw hover highlight
            if (ImGui::IsItemHovered())
            {
                ImGui::GetWindowDrawList()->AddRect(
                    cursor,
                    ImVec2(cursor.x + thumbnailSize, cursor.y + thumbnailSize),
                    IM_COL32(0, 155, 255, 255),
                    4.0f,
                    0,
                    2.0f
                );

                ImGui::BeginTooltip();
                ImGui::Text("Texture: %s", name.c_str());
                ImGui::Text("GUID: %s", texture->getGUID().c_str());
                ImGui::EndTooltip();
            }

            // Handle selection
            if (ImGui::IsItemClicked())
            {
                selectedAssetName = key;
                selectedGameObjectIndex = -1;
                ImGui::CloseCurrentPopup(); // Optional: only if in a popup
            }

            // Center the name below the image
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            float textOffset = std::max(0.0f, (thumbnailSize - textSize.x) * 0.5f);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);
            ImGui::TextWrapped("%s", name.c_str());

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
    }
    else if (selectedAssetCategory == "Scripts")
    {
        for (const auto& [name, factory] : scriptManager.getRegistry()) {
            if (ImGui::Selectable(name.c_str())) {
                auto script = factory();

                ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
                bool isSelected = (selectedAssetName == name);

                if (ImGui::Selectable(name.c_str(), isSelected, flags))
                {
                    selectedAssetName = name;
                    selectedGameObjectIndex = -1;
                }
            }
        }
    }

    renderAssetContextMenu();

    ImGui::End();
}

void se::ImGuiManager::renderPerformancePanel()
{
    auto* scene = sceneManager.getActiveScene();
    if (!scene) return;
    const auto& stats = performance.getStats();
    const float frameMs = static_cast<float>(stats.frameTimeMs);
    const float fps = frameMs > 0.0f ? 1000.0f / frameMs : 0.0f;
    const float physicsMs = static_cast<float>(stats.completedFramePhysicsMs);
    const float renderMs = static_cast<float>(stats.renderBeginMs + stats.renderRecordMs + stats.renderSubmitPresentMs);

    ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.9f);
    if (ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Frame time: %.3f ms", frameMs);
        ImGui::Separator();
        ImGui::Text("Physics update: %.3f ms", physicsMs);
        ImGui::Text("Rendering: %.3f ms", renderMs);
        ImGui::Indent();
        ImGui::Text("Begin / fence wait / acquire: %.3f ms", stats.renderBeginMs);
        ImGui::Text("Recording / uploads / UI: %.3f ms", stats.renderRecordMs);
        ImGui::Text("Submit / present: %.3f ms", stats.renderSubmitPresentMs);
        ImGui::Separator();
        ImGui::Text("Dynamic bodies: %zu", stats.dynamicBodyCount);
        ImGui::Text("Colliders: %zu", stats.colliderCount);
        ImGui::Text("Step time: %.3f ms", stats.lastPhysicsStepMs);
        ImGui::Text("Fixed steps: %zu", stats.physicsStepsThisFrame);
        ImGui::Text("Measured steps: %llu", static_cast<unsigned long long>(stats.measuredSteps));
    }
    ImGui::End();
}

void se::ImGuiManager::renderPropertiesPanel()
{
    if (!showProperties) return;

    auto scene = sceneManager.getActiveScene();
    std::list<std::unique_ptr<SEGameObject>>& gameobjects = scene->getGameObjects();

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    ImGui::SetNextWindowPos(ImVec2(workPos.x + workSize.x - 300, workPos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, workSize.y), ImGuiCond_Always);

    ImGui::Begin("Properties", &showProperties,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (ImGui::BeginTabBar("PropertiesTabBar", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Properties"))
        {
            if (selectedGameObjectIndex >= 0 && selectedGameObjectIndex < gameobjects.size())
            {
                renderGameObjectProperties();
            }
            else if (!selectedAssetName.empty())
            {
                if (selectedAssetCategory == "Meshes")
                {
                    renderMeshProperties();
                }
                else if (selectedAssetCategory == "Materials")
                {
                    renderMaterialProperties();
                }
                else if (selectedAssetCategory == "Textures")
                {
                    renderTextureProperties();
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                    "Select a game object or asset to view properties");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Post Process"))
        {
            renderPostProcessProperties();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}


void se::ImGuiManager::renderPostProcessProperties()
{
    ImGui::Text("Depth of Field");

    auto pp = postprocess.getSettings();

    float focus      = pp.focusDistance;
    float focusRange = pp.focusRange;
    float blurScale  = pp.blurScale;
    float maxBlur    = pp.maxBlur;

    bool dofChanged = false;

    dofChanged |= ImGui::SliderFloat("Focus Distance", &focus, 0.1f, 5.0f);
    dofChanged |= ImGui::SliderFloat("Focus Range", &focusRange, 0.1f, 10.0f);
    dofChanged |= ImGui::SliderFloat("Blur Scale", &blurScale, 0.5f, 5.0f);
    dofChanged |= ImGui::SliderFloat("Max Blur", &maxBlur, 0.0f, 1.0f);

    if (dofChanged)
    {
        pp.focusDistance = focus;
        pp.focusRange    = focusRange;
        pp.blurScale     = blurScale;
        pp.maxBlur       = maxBlur;

        postprocess.updateSettings(pp);
    }

    ImGui::Separator();

    ImGui::Text("Fog");

    bool fogChanged = false;

    bool fogEnabled = (pp.enableFog != 0);

    fogChanged |= ImGui::Checkbox("Enable Fog", &fogEnabled);
    fogChanged |= ImGui::SliderFloat("Fog Start", &pp.fogStart, 0.1f, 20.0f);
    fogChanged |= ImGui::SliderFloat("Fog End", &pp.fogEnd, 10.0f, 100.0f);
    fogChanged |= ImGui::SliderFloat("Fog Intensity", &pp.fogIntensity, 0.0f, 2.0f);
    fogChanged |= ImGui::ColorEdit3("Fog Color", pp.fogColor);

    pp.enableFog = fogEnabled ? 1 : 0;

    if (fogChanged)
    {
        postprocess.updateSettings(pp);
    }

    ImGui::Separator();

    ImGui::Text("Tone Mapping");

    bool toneChanged = false;

    bool tonemapEnabled = (pp.enableTonemap != 0);

    toneChanged |= ImGui::Checkbox("Enable Tonemap", &tonemapEnabled);
    toneChanged |= ImGui::SliderFloat("Exposure", &pp.exposure, 0.1f, 3.0f);

    pp.enableTonemap = tonemapEnabled ? 1 : 0;

    if (toneChanged)
    {
        postprocess.updateSettings(pp);
    }
}

void se::ImGuiManager::renderGameObjectProperties()
{

    auto scene = sceneManager.getActiveScene();
    std::list<std::unique_ptr<SEGameObject>>& gameobjects = scene->getGameObjects();

    auto it = gameobjects.begin();
    std::advance(it, selectedGameObjectIndex);

    auto& gameObject = *it;

    std::strncpy(gameObjectNameBuffer.data(), gameObject->getName().c_str(), gameObjectNameBuffer.size());
    gameObjectNameBuffer.back() = '\0';

    if (ImGui::InputText("Name", gameObjectNameBuffer.data(), gameObjectNameBuffer.size())) {
        gameObject->setName(gameObjectNameBuffer.data());
    }

    ImGui::Separator();

    renderTransformComponent(gameObject);
    renderMeshComponent(gameObject);
	renderMaterialComponent(gameObject);
    renderLightComponent(gameObject);
    renderScriptComponent(gameObject);
    renderParticleEmitterComponent(gameObject);
    renderColliderComponent(gameObject);
    renderRigidbodyComponent(gameObject);
}

void se::ImGuiManager::renderColliderComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (!ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) return;
    ImGui::PushID("ColliderComponent");

    if (auto* collider = gameObject->getCollider())
    {
        int shapeIndex = static_cast<int>(collider->shape.index());
        if (ImGui::Combo("Shape", &shapeIndex, "Box\0Sphere\0Cylinderthe \0"))
        {
            if (shapeIndex == 0) collider->shape = BoxShape{};
            else if (shapeIndex == 1) collider->shape = SphereShape{};
            else collider->shape = CylinderShape{};
        }

        if (auto* box = std::get_if<BoxShape>(&collider->shape))
        {
            if (ImGui::DragFloat3("Half Extents", &box->halfExtents.x, 0.05f, 0.001f, 1000.0f))
                box->halfExtents = glm::max(box->halfExtents, glm::vec3{0.001f});
        }
        else if (auto* sphere = std::get_if<SphereShape>(&collider->shape))
        {
            if (ImGui::DragFloat("Radius", &sphere->radius, 0.05f, 0.001f, 1000.0f))
                sphere->radius = std::max(sphere->radius, 0.001f);
        }

        if (auto* cylinder = std::get_if<CylinderShape>(&collider->shape))
        {
            if (ImGui::DragFloat("Radius", &cylinder->radius, 0.05f, 0.001f, 1000.0f))
                cylinder->radius = std::max(cylinder->radius, 0.001f);
            if (ImGui::DragFloat("Half Height", &cylinder->halfHeight, 0.05f, 0.001f, 1000.0f))
                cylinder->halfHeight = std::max(cylinder->halfHeight, 0.001f);
            ImGui::TextWrapped("Y-axis cylinder using an enclosing box for collision; curved-surface contacts are approximate.");
        }

        ImGui::DragFloat3("Local Offset", &collider->localOffset.x, 0.05f);
        if (ImGui::DragFloat("Friction", &collider->friction, 0.01f, 0.0f, 2.0f))
            collider->friction = std::max(collider->friction, 0.0f);
        ImGui::TextWrapped("Sphere and AABB collision response with friction. Rotation and transform scale are not applied to colliders; set dimensions explicitly.");

        if (ImGui::Button("Remove Collider")) gameObject->removeCollider();
    }
    else if (ImGui::Button("Add Collider"))
    {
        gameObject->setCollider(SECollider{});
    }

    ImGui::PopID();
}

void se::ImGuiManager::renderRigidbodyComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (!ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen)) return;
    ImGui::PushID("RigidbodyComponent");

    if (auto* body = gameObject->getRigidbody())
    {
        ImGui::Checkbox("Static", &body->isStatic);
        ImGui::Checkbox("Use Gravity", &body->useGravity);
        if (ImGui::DragFloat("Mass", &body->mass, 0.1f, 0.001f, 100000.0f))
            body->mass = std::max(body->mass, 0.001f);
        ImGui::DragFloat3("Velocity", &body->velocity.x, 0.1f);
        if (ImGui::Button("Reset Velocity")) body->velocity = glm::vec3{0.0f};

        if (!gameObject->getCollider())
            ImGui::TextWrapped("Add a collider too: the current scene physics update only processes objects with colliders.");

        if (ImGui::Button("Remove Rigidbody")) gameObject->removeRigidbody();
    }
    else if (ImGui::Button("Add Rigidbody"))
    {
        gameObject->setRigidbody(SERigidbody{});
    }

    ImGui::PopID();
}

void se::ImGuiManager::renderTransformComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto transform = gameObject->getTransform();

        float position[3] = { transform.translation.x, transform.translation.y, transform.translation.z };
        float rotation[3] = { transform.rotation.x, transform.rotation.y, transform.rotation.z };
        float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };

        bool changed = false;

        if (ImGui::DragFloat3("Position", position, 0.1f)) {
            changed = true;
        }
        if (ImGui::DragFloat3("Rotation", rotation, 0.1f)) {
            changed = true;
        }
        if (ImGui::DragFloat3("Scale", scale, 0.01f)) {
            changed = true;
        }

        if (changed) {
            TransformComponent newTransform = transform;
            newTransform.translation = { position[0], position[1], position[2] };
            newTransform.rotation = { rotation[0], rotation[1], rotation[2] };
            newTransform.scale = { scale[0], scale[1], scale[2] };
            gameObject->setTransform(newTransform);
        }

    }
}

void se::ImGuiManager::renderMeshComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (ImGui::CollapsingHeader("Mesh"))
    {
        auto mesh = gameObject->getMesh();

        renderMeshSelector(mesh,
            [&gameObject](std::shared_ptr<se::SEMesh> mesh) {
                gameObject->setMesh(mesh);
            });

        if (mesh)
        {
            ImGui::Text("GUID: %s", mesh->getGUID().c_str());
            ImGui::Separator();

            // Mesh-specific properties
            ImGui::Text("Submeshes: %d", mesh->getSubMeshCount());
            ImGui::Text("Vertices: %d", mesh->getVerticesCount());
            ImGui::Text("Triangles: %d", mesh->getIndicesCount());
        }
    }
}

void se::ImGuiManager::renderMaterialComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (ImGui::CollapsingHeader("Material"))
    {
        auto material = gameObject->getMaterial();

        renderMaterialSelector(material,
            [&gameObject](std::shared_ptr<se::SEMaterial> mat) {
                gameObject->setMaterial(mat);
            });

        if (material)
        {
            ImGui::Separator();
            renderMaterialEditor(material);
        }

    }
}

void se::ImGuiManager::renderLightComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (!gameObject->hasLight()) return;

    if (ImGui::CollapsingHeader("Light"))
    {
        auto& light = gameObject->getLight();

        // Light type selector
        static const char* lightTypeNames[] = {"Point", "Directional", "Spot" };
        int typeIndex = static_cast<int>(light.type) - 1;
        if (ImGui::Combo("Type", &typeIndex, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
        {
            light.type = static_cast<se::LightType>(typeIndex+1);
        }

        // Color picker
        if (ImGui::ColorEdit3("Color##", &light.color.x, ImGuiColorEditFlags_Float))
        {
            // Clamp to [0, 1] in case user enters out-of-range values
            light.color.r = std::clamp(light.color.r, 0.0f, 1.0f);
            light.color.g = std::clamp(light.color.g, 0.0f, 1.0f);
            light.color.b = std::clamp(light.color.b, 0.0f, 1.0f);
        }

        // Intensity slider
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f, "%.2f");

        // Direction (for Directional and Spot lights)
        if (light.type == se::LightType::Directional || light.type == se::LightType::Spot)
        {
            ImGui::DragFloat3("Direction", &light.direction.x, 0.1f, -1.0f, 1.0f);
        }

        // Spot angle (for Spot lights)
        if (light.type == se::LightType::Spot)
        {
            ImGui::SliderFloat("Spot Angle", &light.spotAngle, 0.0f, 90.0f, "%.1f deg");
        }
    }
}

void se::ImGuiManager::renderScriptComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    //if (!gameObject->hasScript()) return;

    if (ImGui::CollapsingHeader("Script"))
    {
        auto& script = gameObject->getScript();

        renderScriptSelector(script,
            [this, &gameObject](std::unique_ptr<se::ScriptComponent> selectedScript) {
                auto *scene = sceneManager.getActiveScene();
                if (scene != nullptr)
                {
                    ScriptContext context = scene->scriptContext(resourceManager, inputSystem);
                    gameObject->setScript(std::move(selectedScript), context);
                }
            });
    }

}

void se::ImGuiManager::renderParticleEmitterComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (!ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    if (!gameObject->hasParticleEmitter())
    {
        ImGui::Text("No emitter attached");
        ImGui::Separator();
        ImGui::Text("Create Preset");

        glm::vec3 pos = gameObject->getTransform().translation;

        if (ImGui::Button("Fire")) {
            auto setup = particleSystem.makeFire(pos, 1.0f);
            gameObject->getParticleEmitter() = { setup.emitter, &particleSystem };
        }
        ImGui::SameLine();
        if (ImGui::Button("Rain")) {
            auto setup = particleSystem.makeRain(pos, glm::vec3(10.0f, 0.0f, 10.0f));
            gameObject->getParticleEmitter() = { setup.emitter, &particleSystem };
        }
        ImGui::SameLine();
        if (ImGui::Button("Snow")) {
            auto setup = particleSystem.makeSnow(pos, glm::vec3(10.0f, 0.0f, 10.0f));
            gameObject->getParticleEmitter() = { setup.emitter, &particleSystem };
        }
        ImGui::SameLine();
        if (ImGui::Button("Vortex")) {
            auto setup = particleSystem.makeVortex(pos, glm::vec3(0.0f, 1.0f, 0.0f), 5.0f);
            gameObject->getParticleEmitter() = { setup.emitter, &particleSystem };
        }
        if (ImGui::Button("Black Hole")) {
            auto setup = particleSystem.makeBlackHole(pos, 10.0f);
            gameObject->getParticleEmitter() = { setup.emitter, &particleSystem };
        }

        ImGui::Separator();
        if (ImGui::Button("Create Empty Emitter"))
        {
            se::Emitter em{};
            em.volume.position = glm::vec4(pos, 1.0f);
            em.volume.type     = se::VolumeType::Point;
            auto handle = particleSystem.addEmitter(em);
            gameObject->getParticleEmitter() = { handle, &particleSystem };
        }
        return;
    }

    auto& comp = gameObject->getParticleEmitter();
    if (!comp.isValid())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Invalid emitter handle");
        return;
    }

    auto& emitter = particleSystem.getEmitter(comp.handle);

    // Header controls
    if (ImGui::Button("Remove Emitter", ImVec2(120, 0)))
    {
        particleSystem.removeEmitter(comp.handle);
        comp.handle = se::INVALID_HANDLE;
        comp.system = nullptr;
        return;
    }

    ImGui::SameLine();
    bool active = (emitter.active != 0);
    if (ImGui::Checkbox("Active", &active))
        emitter.active = active ? 1 : 0;

    ImGui::Separator();

    ImGui::Text("Spawn Volume");
    int volType = static_cast<int>(emitter.volume.type);
    const char* volNames[] = { "Point", "Sphere", "Box", "Cone", "Ring" };
    if (ImGui::Combo("Shape", &volType, volNames, IM_ARRAYSIZE(volNames)))
        emitter.volume.type = static_cast<se::VolumeType>(volType);

    ImGui::DragFloat3("Position##Volume", &emitter.volume.position.x, 0.1f);
    ImGui::DragFloat4("Extents",  &emitter.volume.extents.x,  0.1f);
    ImGui::DragFloat3("Direction", &emitter.volume.direction.x, 0.01f);

    ImGui::Separator();

    ImGui::Text("Emission");
    ImGui::DragFloat("Emit Rate",   &emitter.emitRate,    1.0f, 0.0f, 10000.0f);
    ImGui::DragFloat("Speed Min",   &emitter.speedMin,    0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Speed Max",   &emitter.speedMax,    0.1f, 0.0f, 100.0f);

    ImGui::DragFloat("Life Min",    &emitter.lifetimeMin, 0.1f, 0.01f, 60.0f);
    ImGui::DragFloat("Life Max",    &emitter.lifetimeMax, 0.1f, 0.01f, 60.0f);

    ImGui::DragFloat("Size Start",  &emitter.sizeStart,   0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Size End",    &emitter.sizeEnd,     0.1f, 0.0f, 100.0f);

    ImGui::ColorEdit4("Color Start", &emitter.colorStart.x);
    ImGui::ColorEdit4("Color End",   &emitter.colorEnd.x);

    ImGui::DragFloat3("Emit Direction", &emitter.direction.x, 0.01f);
    ImGui::DragFloat("Spread (rad)",    &emitter.direction.w, 0.01f, 0.0f, 6.283f);

    ImGui::Separator();
    ImGui::Text("Attached Forces: %u / 8", emitter.forceCount);
    for (uint32_t i = 0; i < emitter.forceCount; ++i)
    {
        ImGui::BulletText("Force %u", emitter.forceIndices[i]);
    }
}

void se::ImGuiManager::renderMeshProperties()
{
    auto meshes = resourceManager.getMeshes();
    auto it = meshes->find(selectedAssetName);
    if (it != meshes->end())
    {
        auto mesh = it->second;

        std::strncpy(meshNameBuffer.data(), mesh->getName().c_str(), meshNameBuffer.size());
        meshNameBuffer.back() = '\0';

        ImGui::Text("Mesh: %s", mesh->getName().c_str());
        ImGui::SameLine();

        if (ImGui::InputText("##Name", meshNameBuffer.data(), meshNameBuffer.size())) {
            mesh->setName(meshNameBuffer.data());
        }

        ImGui::Text("GUID: %s", mesh->getGUID().c_str());
        ImGui::Separator();

        // Mesh-specific properties
        ImGui::Text("Submeshes: %d", mesh->getSubMeshCount());
        ImGui::Text("Vertices: %d", mesh->getVerticesCount());
        ImGui::Text("Triangles: %d", mesh->getIndicesCount());
    }
}

void se::ImGuiManager::renderMaterialProperties()
{
    auto materials = resourceManager.getMaterials();
    auto it = materials->find(selectedAssetName);
    if (it != materials->end())
    {
        auto material = it->second;

        std::strncpy(materialNameBuffer.data(), material->getName().c_str(), materialNameBuffer.size());
        materialNameBuffer.back() = '\0';

        ImGui::Text("Material: %s", material->getName().c_str());
		ImGui::SameLine();

        if (ImGui::InputText("##Name", materialNameBuffer.data(), materialNameBuffer.size())) {
            material->setName(materialNameBuffer.data());
        }

        ImGui::Text("GUID: %s", material->getGUID().c_str());
        ImGui::Text("Type: PBR");
        ImGui::Separator();

        renderMaterialEditor(material);
    }
}

void se::ImGuiManager::renderTextureProperties()
{
    auto textures = resourceManager.getTextures();
    auto it = textures->find(selectedAssetName);
    if (it != textures->end())
    {
        auto texture = it->second;

        std::strncpy(textureNameBuffer.data(), texture->getName().c_str(), textureNameBuffer.size());
        textureNameBuffer.back() = '\0';

        ImGui::Text("Texture: %s", texture->getName().c_str());
        ImGui::SameLine();

        if (ImGui::InputText("##Name", textureNameBuffer.data(), textureNameBuffer.size())) {
            texture->setName(textureNameBuffer.data());
        }

        ImGui::Text("GUID: %s", texture->getGUID().c_str());
        ImGui::Separator();

        // Texture-specific properties would go here
        ImGui::Text("Format: RGBA");
        ImGui::Text("Mip Levels: N/A"); // Would need to expose this from SETexture
        ImGui::Text("Filter Mode: Linear");
        ImGui::Text("Wrap Mode: Repeat");

        ImGui::Image((ImTextureID)texture->getTextureDescriptorSet(), ImVec2(128, 128));
    }
}

void se::ImGuiManager::renderMaterialEditor(std::shared_ptr<se::SEMaterial> material)
{
    if (!material) return;

    // PBR Properties
	glm::vec3 color = material->getColor();
    float metallic = material->getMetallic();
    float roughness = material->getRoughness();
    float ao = material->getAO();
    float heightScale = material->getHeightScale();
    glm::vec2 uvTiling = material->getUVScale();
    
    // Color picker
    if (ImGui::ColorEdit3("Color", &color.x, ImGuiColorEditFlags_Float))
    {
        // Clamp to [0, 1] in case user enters out-of-range values
        color.r = std::clamp(color.r, 0.0f, 1.0f);
        color.g = std::clamp(color.g, 0.0f, 1.0f);
        color.b = std::clamp(color.b, 0.0f, 1.0f);

		material->setColor(color);
    }

    if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
    {
        material->setMetallic(metallic);
    }

    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
    {
        material->setRoughness(roughness);
    }

    if (ImGui::SliderFloat("Ambient Occlusion", &ao, 0.0f, 1.0f))
    {
        material->setAO(ao);
    }

    if (ImGui::SliderFloat("Height Scale", &heightScale, 0.0f, 0.1f))
    {
        material->setHeightScale(heightScale);
    }

    if(ImGui::DragFloat2("UV Tiling", &uvTiling.x, 0.05f, 0.01f, 100.0f))
    {
        material->setUVScale(uvTiling);
    }

    ImGui::Separator();
    ImGui::Text("Textures:");

    // Diffuse Texture
    renderTextureSelector("Diffuse", material->getDiffuseTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setDiffuseTexture(tex);
        });
    ImGui::Separator();
    // Normal Texture
    renderTextureSelector("Normal", material->getNormalTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setNormalTexture(tex);
        });
    ImGui::Separator();
    // Metallic Texture
    renderTextureSelector("Metallic", material->getMetallicTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setMetallicTexture(tex);
        });
    ImGui::Separator();
    // Roughness Texture
    renderTextureSelector("Roughness", material->getRoughnessTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setRoughnessTexture(tex);
        });
    ImGui::Separator();
    // AO Texture
    renderTextureSelector("AO", material->getAOTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setAOTexture(tex);
        });
    // AO Texture
    renderTextureSelector("Height", material->getHeightTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setHeightTexture(tex);
        });
}

void se::ImGuiManager::renderTextureSelector(const std::string& label,
    std::shared_ptr<se::SETexture> currentTexture,
    std::function<void(std::shared_ptr<se::SETexture>)> onTextureSelected)
{
    ImGui::PushID(label.c_str());

    const float thumbnailSize = 24;
    const ImVec2 imageSize(thumbnailSize, thumbnailSize);
    bool openPopup = false;

    ImGui::BeginGroup();

    // Thumbnail image
    if (currentTexture) {
        ImTextureID descriptor = (ImTextureID)currentTexture->getTextureDescriptorSet();
        if (ImGui::ImageButton(("##" + label).c_str(), descriptor, imageSize)) {
            openPopup = true;
        }
    }
    else {
        // Empty/placeholder preview
        ImVec4 borderColor = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        if (ImGui::InvisibleButton("NoTexture", imageSize)) {
            openPopup = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("No texture assigned");
        }
        ImGui::GetWindowDrawList()->AddRect(
            ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(borderColor));
    }

    ImGui::SameLine();

    // Label (e.g., "Diffuse", "Normal") as clickable
    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(100, 0))) {
        openPopup = true;
    }

    ImGui::EndGroup();

    if (openPopup) {
        ImGui::OpenPopup("Select Texture");
    }

    // Texture selection popup
    if (ImGui::BeginPopup("Select Texture"))
    {
        ImGui::InputTextWithHint("##Search", "Search textures...",
            textureSelectorSearch.data(), textureSelectorSearch.size());

        ImGui::Separator();
        const float thumbnailSize = 64.0f;
        const float padding = 8.0f;
        float cellSize = thumbnailSize + padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        ImGui::BeginChild("TextureGrid", ImVec2(0, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(columnCount, nullptr, false);

        auto textures = resourceManager.getTextures();
        for (const auto& [key, texture] : *textures)
        {
            const std::string& name = texture->getName().empty() ? key : texture->getName();
            if (textureSelectorSearch[0] != '\0' &&
                name.find(textureSelectorSearch.data()) == std::string::npos)
                continue;

            ImGui::PushID(texture.get());

            ImVec2 imagePos = ImGui::GetCursorScreenPos();
            ImGui::Image((ImTextureID)texture->getTextureDescriptorSet(), { thumbnailSize, thumbnailSize });

            if (ImGui::IsItemHovered())
            {
                ImGui::GetWindowDrawList()->AddRect(
                    imagePos,
                    ImVec2(imagePos.x + thumbnailSize, imagePos.y + thumbnailSize),
                    IM_COL32(0, 155, 255, 255), // Blue border
                    4.0f,                       // Corner rounding
                    0,
                    2.0f                        // Thickness
                );

                ImGui::BeginTooltip();
                ImGui::Text("Texture: %s", name.c_str());
                ImGui::Text("GUID: %s", texture->getGUID().c_str());
                ImGui::EndTooltip();
            }

            if (ImGui::IsItemClicked())
            {
                onTextureSelected(texture);
                ImGui::CloseCurrentPopup();
            }

            ImGui::TextWrapped("%s", name.c_str());

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::EndPopup();
    }


    ImGui::PopID();
}

void se::ImGuiManager::renderMaterialSelector(
    std::shared_ptr<se::SEMaterial> currentMaterial,
    std::function<void(std::shared_ptr<se::SEMaterial>)> onMaterialSelected)
{
	std::string label = currentMaterial ? currentMaterial->getName() : "No Material";
    ImGui::PushID(label.c_str());

    bool openPopup = false;

    ImGui::BeginGroup();

    // Label (e.g., "Diffuse", "Normal") as clickable
    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(100, 0))) {
        openPopup = true;
    }

    ImGui::EndGroup();

    if (openPopup) {
        ImGui::OpenPopup("Select Material");
    }

    // Texture selection popup
    if (ImGui::BeginPopup("Select Material"))
    {
        ImGui::InputTextWithHint("##Search", "Search materials...",
            materialSelectorSearch.data(), materialSelectorSearch.size());

        ImGui::Separator();

        ImGui::BeginChild("TextureGrid", ImVec2(0, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(1, nullptr, false);

        if (ImGui::Selectable("None", false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, 0)))
        {
            onMaterialSelected(nullptr);
            ImGui::CloseCurrentPopup();
        }

        auto materials = resourceManager.getMaterials();
        for (const auto& [key, material] : *materials)
        {
            const std::string& name = material->getName().empty() ? key : material->getName();
            if (materialSelectorSearch[0] != '\0' &&
                name.find(materialSelectorSearch.data()) == std::string::npos)
                continue;

            ImGui::PushID(material.get());

            if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, 0)))
            {
                onMaterialSelected(material);
                ImGui::CloseCurrentPopup();
            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::EndPopup();
    }


    ImGui::PopID();
}

void se::ImGuiManager::renderMeshSelector(
    std::shared_ptr<se::SEMesh> currentMesh,
    std::function<void(std::shared_ptr<se::SEMesh>)> onMeshSelected)
{
    std::string label = currentMesh ? currentMesh->getName() : "No Mesh";
    ImGui::PushID(label.c_str());

    bool openPopup = false;

    ImGui::BeginGroup();

    // Label (e.g., "Diffuse", "Normal") as clickable
    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(100, 0))) {
        openPopup = true;
    }

    ImGui::EndGroup();

    if (openPopup) {
        ImGui::OpenPopup("Select Mesh");
    }

    // Texture selection popup
    if (ImGui::BeginPopup("Select Mesh"))
    {
        ImGui::InputTextWithHint("##Search", "Search meshes...",
            meshSelectorSearch.data(), meshSelectorSearch.size());

        ImGui::Separator();

        ImGui::BeginChild("TextureGrid", ImVec2(0, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(1, nullptr, false);

        auto meshes = resourceManager.getMeshes();
        for (const auto& [key, mesh] : *meshes)
        {
            const std::string& name = mesh->getName().empty() ? key : mesh->getName();
            if (meshSelectorSearch[0] != '\0' &&
                name.find(meshSelectorSearch.data()) == std::string::npos)
                continue;

            ImGui::PushID(mesh.get());

            if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, 0)))
            {

                onMeshSelected(mesh);
                ImGui::CloseCurrentPopup();

            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::EndPopup();
    }


    ImGui::PopID();
}

void se::ImGuiManager::renderScriptSelector(
    std::unique_ptr<se::ScriptComponent>& currentScript,
    std::function<void(std::unique_ptr<se::ScriptComponent>)> onScriptSelected)
{
    std::string label = currentScript ? currentScript->getName() : "No Script";
    ImGui::PushID(label.c_str());

    bool openPopup = false;

    ImGui::BeginGroup();

    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(100, 0))) {
        openPopup = true;
    }

    ImGui::EndGroup();

    if (openPopup) {
        ImGui::OpenPopup("Select Script");
    }

    if (ImGui::BeginPopup("Select Script"))
    {
        ImGui::InputTextWithHint("##Search", "Search scripts...",
            scriptSelectorSearch.data(), scriptSelectorSearch.size());
        ImGui::Separator();

        ImGui::BeginChild("ScriptList", ImVec2(0, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(1, nullptr, false);

        // Option to remove script
        if (ImGui::Selectable("None", false)) {
            onScriptSelected(nullptr);
            ImGui::CloseCurrentPopup();
        }

        // Iterate over all registered scripts
        for (const auto& [name, factory] : scriptManager.getRegistry())
        {
            if (scriptSelectorSearch[0] != '\0' &&
                name.find(scriptSelectorSearch.data()) == std::string::npos)
                continue;

            ImGui::PushID(name.c_str());

            if (ImGui::Selectable(name.c_str(), false)) {
                onScriptSelected(factory());
                ImGui::CloseCurrentPopup();
            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::EndPopup();
    }

    ImGui::PopID();
}


se::ImGuiManager::ImGuiManager(SEDevice &seDevice, VkRenderPass renderPass, GLFWwindow &window,
    uint32_t swapChainImageCount, ResourceManager &resources, SceneManager &scenes,
    SEPostProcess &postProcess, ParticleSystem &particles, SEInputSystem &input,
    ScriptManager& scripts, const PerformanceMonitor& performance)
    : resourceManager{resources}, sceneManager{scenes}, postprocess{postProcess},
      particleSystem{particles}, inputSystem{input}, scriptManager{scripts}, performance{performance}
{
    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    contextCreated = true;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer bindings
    try
    {
    if (!ImGui_ImplGlfw_InitForVulkan(&window, true))
    {
        throw std::runtime_error("ImGui GLFW backend initialization failed");
    }
    glfwBackendInitialized = true;
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = seDevice.getInstance();
    initInfo.PhysicalDevice = seDevice.physicaldevice();
    initInfo.Device = seDevice.device();
    initInfo.QueueFamily = seDevice.findPhysicalQueueFamilies().graphicsFamily.value();
    initInfo.Queue = seDevice.graphicsQueue();
    initInfo.RenderPass = renderPass;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = seDevice.getDescriptorPool();
    initInfo.MinImageCount = 2;
    // ImGui owns a rotating vertex/index buffer pair per slot. Keep that ring
    // at least as large as the engine flight window to avoid reusing an in-use slot.
    initInfo.ImageCount = std::max(swapChainImageCount, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = checkImGuiVulkanResult;
    if (!ImGui_ImplVulkan_Init(&initInfo))
    {
        throw std::runtime_error("ImGui Vulkan backend initialization failed");
    }
    vulkanBackendInitialized = true;
    // Load Fonts
    io.Fonts->AddFontDefault();
    }
    catch (...)
    {
        cleanup();
        throw;
    }
}

se::ImGuiManager::~ImGuiManager()
{
    cleanup();
}

void se::ImGuiManager::newFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void se::ImGuiManager::render(VkCommandBuffer commandBuffer)
{
    renderSceneHierarchy();
    renderAssetBrowser();
    renderAssetViewer();
    renderPropertiesPanel();
    renderPerformancePanel();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void se::ImGuiManager::cleanup()
{
    if (vulkanBackendInitialized)
    {
        ImGui_ImplVulkan_Shutdown();
        vulkanBackendInitialized = false;
    }
    if (glfwBackendInitialized)
    {
        ImGui_ImplGlfw_Shutdown();
        glfwBackendInitialized = false;
    }
    if (contextCreated)
    {
        ImGui::DestroyContext();
        contextCreated = false;
    }
}
