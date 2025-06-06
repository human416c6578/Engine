#include "se_window.hpp"
#include "se_device.hpp"
#include "se_renderer.hpp"
#include "se_gameobject.hpp"
#include "se_camera.hpp"
#include "se_pbr.hpp"
#include "se_material_system.hpp"
#include "se_texture_system.hpp"
#include "se_mesh_system.hpp"
#include "se_resource_manager.hpp"

#include <exception>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include "imgui_manager.hpp"


const int WIDTH = 800;
const int HEIGHT = 600;

class App
{
public:
    App()
    {
		PBR = std::make_unique<se::PBR>(seDevice, seRenderer.getSwapChainRenderPass(), seCubemap);
        MaterialSystem = std::make_shared<se::MaterialSystem>(seDevice, PBR->getPipelineLayout(), PBR->getPipeline(), PBR->getMaterialDescriptorSetLayout());
		TextureSystem = std::make_shared<se::TextureSystem>(seDevice);
		MeshSystem = std::make_shared<se::MeshSystem>(seDevice);

		ResourceManager = std::make_shared<se::ResourceManager>();

        ResourceManager->setTextureSystem(TextureSystem);
		ResourceManager->setMaterialSystem(MaterialSystem);
        ResourceManager->setMeshSystem(MeshSystem);

		imguiManager.init(seDevice, seRenderer.getSwapChainRenderPass(), seWindow.getGLFWwindow(), &gameObjects, ResourceManager.get());


    }
    
    ~App() {}

    App(const App &) = delete;
    App &operator=(const App &) = delete;
    
    void run()
    {
        mainLoop();
    }

private:
    se::SEWindow seWindow{WIDTH, HEIGHT, "Vulkan"};
    se::SEDevice seDevice{seWindow};
    se::SERenderer seRenderer{seWindow, seDevice};
    se::SECubemap seCubemap{ seDevice, seRenderer, "hdr/rostock_laage_airport_2k.hdr" };

    std::shared_ptr<se::ResourceManager> ResourceManager;

    std::unique_ptr<se::PBR> PBR;
    std::shared_ptr<se::MaterialSystem> MaterialSystem;
    std::shared_ptr<se::TextureSystem> TextureSystem;
    std::shared_ptr<se::MeshSystem> MeshSystem;
	se::ImGuiManager imguiManager;

    void loadGameObjects();
    void updateGameObjects(float dt);
    void mainLoop();
    std::vector<se::SEGameObject> gameObjects;
};
