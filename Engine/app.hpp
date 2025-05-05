#include "se_window.hpp"
#include "se_device.hpp"
#include "se_renderer.hpp"
#include "se_gameobject.hpp"
#include "se_camera.hpp"
#include "se_pbr.hpp"

#include <exception>
#include <cstdlib>
#include <iostream>
#include <chrono>

const int WIDTH = 800;
const int HEIGHT = 600;

class App
{
public:
    App()
    {
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

    void loadGameObjects(VkDescriptorSetLayout descriptorSetLayout);
    void updateGameObjects(float dt);
    void mainLoop();
    std::vector<se::SEGameObject> gameObjects;
};
