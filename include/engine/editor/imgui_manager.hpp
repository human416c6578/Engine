#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

namespace se
{
    class SEDevice;
    class ResourceManager;
    class SceneManager;
    class SEPostProcess;
    class ParticleSystem;
    class SEInputSystem;
    class SEGameObject;
    class SEMaterial;
    class SETexture;
    class SEMesh;
    class ScriptComponent;
    class ScriptManager;

    class ImGuiManager
    {
    public:
        ImGuiManager(SEDevice& device, VkRenderPass renderPass, GLFWwindow& window,
            uint32_t swapChainImageCount, ResourceManager& resources,
            SceneManager& scenes, SEPostProcess& postProcess,
            ParticleSystem& particles, SEInputSystem& input, ScriptManager& scripts);
        ~ImGuiManager();

        ImGuiManager(const ImGuiManager&) = delete;
        ImGuiManager& operator=(const ImGuiManager&) = delete;
        ImGuiManager(ImGuiManager&&) = delete;
        ImGuiManager& operator=(ImGuiManager&&) = delete;

        void newFrame();
        void render(VkCommandBuffer commandBuffer);
        void cleanup();

    private:
        void renderSceneHierarchy();
        void renderAssetBrowser();
        void renderAssetContextMenu();
        void renderAssetViewer();
        void renderPropertiesPanel();
        void renderGameObjectProperties();
        void renderTransformComponent(std::unique_ptr<SEGameObject>& gameObject);
        void renderMeshComponent(std::unique_ptr<SEGameObject>& gameObject);
        void renderMaterialComponent(std::unique_ptr<SEGameObject>& gameObject);
        void renderLightComponent(std::unique_ptr<SEGameObject>& gameObject);
        void renderScriptComponent(std::unique_ptr<SEGameObject>& gameObject);
        void renderParticleEmitterComponent(std::unique_ptr<SEGameObject>& gameObject);
        void renderMeshProperties();
        void renderMaterialProperties();
        void renderTextureProperties();
        void renderPostProcessProperties();
        void renderMaterialEditor(std::shared_ptr<SEMaterial> material);
        void renderTextureSelector(const std::string& label,
            std::shared_ptr<SETexture> currentTexture,
            std::function<void(std::shared_ptr<SETexture>)> onTextureSelected);
        void renderMaterialSelector(std::shared_ptr<SEMaterial> currentMaterial,
            std::function<void(std::shared_ptr<SEMaterial>)> onMaterialSelected);
        void renderMeshSelector(std::shared_ptr<SEMesh> currentMesh,
            std::function<void(std::shared_ptr<SEMesh>)> onMeshSelected);
        void renderScriptSelector(std::unique_ptr<ScriptComponent>& currentScript,
            std::function<void(std::unique_ptr<ScriptComponent>)> onScriptSelected);

        ResourceManager& resourceManager;
        SceneManager& sceneManager;
        SEPostProcess& postprocess;
        ParticleSystem& particleSystem;
        SEInputSystem& inputSystem;
        ScriptManager& scriptManager;
        bool contextCreated{false};
        bool glfwBackendInitialized{false};
        bool vulkanBackendInitialized{false};

        int selectedGameObjectIndex{-1};
        std::string selectedAssetCategory{"Meshes"};
        std::string selectedAssetName;
        std::vector<std::string> assetCategories{"Meshes", "Materials", "Textures", "Scripts"};
        bool showSceneHierarchy{true};
        bool showAssetBrowser{true};
        bool showAssetViewer{true};
        bool showProperties{true};

        int cubeCount{0};
        int sphereCount{0};
        std::array<char, 128> assetBrowserTextureSearch{};
        std::array<char, 128> textureSelectorSearch{};
        std::array<char, 128> materialSelectorSearch{};
        std::array<char, 128> meshSelectorSearch{};
        std::array<char, 128> scriptSelectorSearch{};
        std::array<char, 128> gameObjectNameBuffer{};
        std::array<char, 128> meshNameBuffer{};
        std::array<char, 128> materialNameBuffer{};
        std::array<char, 128> textureNameBuffer{};
    };
}
