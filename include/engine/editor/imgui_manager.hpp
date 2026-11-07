#pragma once
#include "engine/vulkan/se_device.hpp"
#include "engine/platform/se_window.hpp"
#include "engine/renderer/se_renderer.hpp"
#include "engine/renderer/se_postprocess.hpp"
#include "engine/scene/se_gameobject.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/scene/se_scene_manager.hpp"
#include <imgui/imgui.h>
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui_filedialog/ImGuiFileDialog.h"


namespace se
{
    class ImGuiManager
    {
    public:
        ImGuiManager() = default;
        ~ImGuiManager() = default;

        void init(SEDevice& seDevice, VkRenderPass renderPass, GLFWwindow* window, uint32_t swapChainImageCount, se::ResourceManager* resourceManager, se::SEPostProcess* postprocess, se::ParticleSystem* particleSystem);

        void newFrame();
        void render(VkCommandBuffer commandBuffer);
        void cleanup();

    private:
        // Core rendering methods
        void renderSceneHierarchy();
        void renderAssetBrowser();
        void renderAssetContextMenu();
        void renderAssetViewer();
        void renderPropertiesPanel();

        // GameObject properties
        void renderGameObjectProperties();
        void renderTransformComponent(std::unique_ptr<se::SEGameObject>& gameObject);
        void renderMeshComponent(std::unique_ptr<se::SEGameObject>& gameObject);
        void renderMaterialComponent(std::unique_ptr<se::SEGameObject>& gameObject);
        void renderLightComponent(std::unique_ptr<se::SEGameObject>& gameObject);
        void renderScriptComponent(std::unique_ptr<se::SEGameObject>& gameObject);
        void renderParticleEmitterComponent(std::unique_ptr<se::SEGameObject>& gameObject);


        // Asset properties
        void renderMeshProperties();
        void renderMaterialProperties();
        void renderTextureProperties();
        void renderPostProcessProperties();

        // Utility methods
        void renderMaterialEditor(std::shared_ptr<se::SEMaterial> material);
        void renderTextureSelector(const std::string& label, std::shared_ptr<se::SETexture> currentTexture, std::function<void(std::shared_ptr<se::SETexture>)> onTextureSelected);

        void renderMaterialSelector(std::shared_ptr<se::SEMaterial> currentMaterial, std::function<void(std::shared_ptr<se::SEMaterial>)> onMaterialSelected);

        void renderMeshSelector(std::shared_ptr<se::SEMesh> currentMesh, std::function<void(std::shared_ptr<se::SEMesh>)> onMeshSelected);

        void renderScriptSelector(std::unique_ptr<se::ScriptComponent>& currentScript, std::function<void(std::unique_ptr<se::ScriptComponent>)> onScriptSelected);

        // Vulkan/ImGui setup
        GLFWwindow* window{ nullptr };
        VkInstance instance{ VK_NULL_HANDLE };
        VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
        VkDevice device{ VK_NULL_HANDLE };
        uint32_t graphicsFamily{ 0 };
        VkQueue graphicsQueue{ VK_NULL_HANDLE };
        VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
        VkRenderPass renderPass{ VK_NULL_HANDLE };
        uint32_t imageCount{ 0 };

        // Data references
        se::ResourceManager* resourceManager{ nullptr };
        se::SceneManager* sceneManager{ nullptr };
        se::SEPostProcess* postprocess{ nullptr };
        se::ParticleSystem* particleSystem = nullptr;


        // Selection state
        int selectedGameObjectIndex = -1;
        std::string selectedAssetCategory = "Meshes";
        std::string selectedAssetName = "";

        // Asset categories
        std::vector<std::string> assetCategories = { "Meshes", "Materials", "Textures", "Scripts"};

        // UI flags
        bool showSceneHierarchy = true;
        bool showAssetBrowser = true;
        bool showAssetViewer = true;
        bool showProperties = true;
    };
}
