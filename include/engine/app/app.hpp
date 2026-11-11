#pragma once

#include "engine/platform/se_window.hpp" // SEWindow is an App member stored by value.
#include "engine/platform/se_input_system.hpp" // SEInputSystem is an App member stored by value.
#include "engine/vulkan/se_device.hpp" // SEDevice is an App member stored by value.
#include "engine/renderer/se_renderer.hpp" // SERenderer is an App member stored by value.

#include "engine/core/se_performance.hpp"

#include <memory>

namespace se
{
    class SECubemap;
    class PBR;
    class ParticleSystem;
    class SEPostProcess;
    class TextureSystem;
    class MaterialSystem;
    class MeshSystem;
    class ResourceManager;
    class SceneManager;
    class ImGuiManager;
    class ScriptManager;
    class SEPhysics;
}

class App
{
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    void run() { mainLoop(); }

private:
    se::SEWindow seWindow{800, 600, "Vulkan"};
    se::SEInputSystem inputSystem{*seWindow.getGLFWwindow()};
    se::SEDevice seDevice{seWindow};
    se::SERenderer seRenderer{seWindow, seDevice};
    se::PerformanceMonitor performance;
    std::unique_ptr<se::SECubemap> seCubemap;

    std::unique_ptr<se::PBR> PBR;
    std::unique_ptr<se::ParticleSystem> ParticleSystem;
    std::unique_ptr<se::SEPostProcess> PostProcess;
    std::unique_ptr<se::TextureSystem> TextureSystem;
    std::unique_ptr<se::MaterialSystem> MaterialSystem;
    std::unique_ptr<se::MeshSystem> MeshSystem;
    std::unique_ptr<se::ResourceManager> ResourceManager;
    std::unique_ptr<se::ScriptManager> scriptManager;
    std::unique_ptr<se::SceneManager> sceneManager;
    std::unique_ptr<se::ImGuiManager> imguiManager;

    void mainLoop();
};
