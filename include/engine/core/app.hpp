#include "engine/core/se_window.hpp"
#include "engine/core/se_device.hpp"
#include "engine/renderer/se_renderer.hpp"
#include "engine/scene/se_gameobject.hpp"
#include "engine/core/se_camera.hpp"
#include "engine/renderer/se_pbr.hpp"
#include "engine/renderer/se_particle_system.hpp"
#include "engine/renderer/se_postprocess.hpp"
#include "engine/renderer/se_material_system.hpp"
#include "engine/renderer/se_texture_system.hpp"
#include "engine/renderer/se_mesh_system.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/core/se_input_system.hpp"

#include <exception>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include "engine/ui/imgui_manager.hpp"
#include "engine/scene/se_scene_manager.hpp"


const int WIDTH = 800;
const int HEIGHT = 600;

class App
{
public:
    App()
    {
        se::SEInputSystem::initialize(seWindow.getGLFWwindow());

		PBR = std::make_unique<se::PBR>(seDevice, seRenderer.getOffscreenRenderer()->getRenderPass(), seCubemap);
        ParticleSystem = std::make_unique<se::ParticleSystem>(seDevice, seRenderer.getOffscreenRenderer()->getRenderPass());
        PostProcess = std::make_shared<se::SEPostProcess>(seDevice, seRenderer.getSwapChainRenderPass());
        MaterialSystem = std::make_shared<se::MaterialSystem>(seDevice, PBR->getPipelineLayout(), PBR->getPipeline(), PBR->getMaterialDescriptorSetLayout());
		TextureSystem = std::make_shared<se::TextureSystem>(seDevice);
		MeshSystem = std::make_shared<se::MeshSystem>(seDevice);

		ResourceManager = std::make_unique<se::ResourceManager>();

        ResourceManager->setTextureSystem(TextureSystem);
		ResourceManager->setMaterialSystem(MaterialSystem);
        ResourceManager->setMeshSystem(MeshSystem);

        sceneManager = &se::SceneManager::getInstance();
        sceneManager->setResourceManager(ResourceManager.get());
        auto& mainScene = sceneManager->createScene("MainScene");
        sceneManager->setActiveScene("MainScene");

		imguiManager.init(seDevice, seRenderer.getSwapChainRenderPass(), seWindow.getGLFWwindow(), ResourceManager.get(), PostProcess.get());
    }

    ~App() {
        if (sceneManager) {
            sceneManager->destroyAllScenes();
        }
    }

    App(const App &) = delete;
    App &operator=(const App &) = delete;

    void run()
    {
        mainLoop();
    }

private:
    se::SEWindow seWindow{ WIDTH, HEIGHT, "Vulkan" };
    se::SEDevice seDevice{ seWindow };
    se::SERenderer seRenderer{ seWindow, seDevice };
    se::SECubemap seCubemap{ seDevice, seRenderer, "assets/hdr/snowy_forest_4k.hdr" };

    std::unique_ptr<se::ResourceManager> ResourceManager;
    std::unique_ptr<se::PBR> PBR;
    std::unique_ptr<se::ParticleSystem> ParticleSystem;
    std::shared_ptr<se::SEPostProcess> PostProcess;
    std::shared_ptr<se::TextureSystem> TextureSystem;
    std::shared_ptr<se::MaterialSystem> MaterialSystem;
    std::shared_ptr<se::MeshSystem> MeshSystem;
    se::SceneManager* sceneManager;
    se::ImGuiManager imguiManager;

    void mainLoop();

};
