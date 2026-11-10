#include "engine/app/app.hpp"
#include "engine/vulkan/se_vk_result.hpp"
#include "engine/core/se_logger.hpp"
#include <string>
#include <memory>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <chrono>
#include "engine/scene/se_gameobject.hpp"
#include "engine/scene/se_scene.hpp"
#include "engine/editor/imgui_manager.hpp"
#include "engine/scene/se_scene_manager.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/renderer/se_mesh_system.hpp"
#include "engine/renderer/se_texture_system.hpp"
#include "engine/renderer/se_postprocess.hpp"
#include "engine/renderer/se_particle_system.hpp"
#include "engine/renderer/se_pbr.hpp"
#include "engine/renderer/se_offscreen_renderer.hpp"
#include "engine/renderer/se_cubemap.hpp"
#include <GLFW/glfw3.h>
#include <cstdio>
#include "engine/platform/keyboard_movement_controller.hpp"
#include "engine/renderer/se_material_system.hpp"
#include "engine/scripting/ExampleScript.hpp"
#include "engine/scripting/TestScript.hpp"
#include "engine/scripting/Snake.hpp"
#include "engine/scripting/TestPhysics.hpp"
#include "engine/scripting/se_script_manager.hpp"

namespace
{
    void closeFrameDisposals(se::SEDevice& device, se::SERenderer& renderer, const char* operation) noexcept
    {
        renderer.stopFrameSubmission();
        const auto result = vkDeviceWaitIdle(device.device());
        device.logResult(result, operation);
        // Unknown completion cannot permit ordinary GPU-owner unwinding.
        if (result != VK_SUCCESS) std::terminate();
        renderer.discardFrameRecordingAfterIdle();
        device.closeDisposalsAfterIdle();
    }
}

App::StartupTag App::initializeLogging()
{
    se::log::initialize(se::log::Level::Debug);
    return {};
}

App::App() : App{initializeLogging()} {}

App::App(StartupTag)
{
    try
    {
    seCubemap = std::make_unique<se::SECubemap>(seDevice, seRenderer, "assets/hdr/snowy_forest_4k.hdr");
    PBR = std::make_unique<se::PBR>(seDevice, seRenderer.getOffscreenRenderer()->getRenderPass(), *seCubemap);
    ParticleSystem = std::make_unique<se::ParticleSystem>(seDevice, seRenderer.getOffscreenRenderer()->getRenderPass());
    PostProcess = std::make_unique<se::SEPostProcess>(seDevice, seRenderer.getSwapChainRenderPass());
    TextureSystem = std::make_unique<se::TextureSystem>(seDevice);
    MaterialSystem = std::make_unique<se::MaterialSystem>(
        seDevice, *TextureSystem, PBR->getPipelineLayout(), PBR->getPipeline(),
        PBR->getMaterialDescriptorSetLayout());
    MeshSystem = std::make_unique<se::MeshSystem>(seDevice, *MaterialSystem, *TextureSystem);

    ResourceManager = std::make_unique<se::ResourceManager>(
        *TextureSystem, *MaterialSystem, *MeshSystem);

    scriptManager = std::make_unique<se::ScriptManager>();
    scriptManager->registerScript<se::ExampleScript>("ExampleScript");
    scriptManager->registerScript<se::TestScript>("TestScript");
    scriptManager->registerScript<se::SnakeScript>("SnakeScript");
    scriptManager->registerScript<se::TestPhysics>("TestPhysics");

    sceneManager = std::make_unique<se::SceneManager>(
        *ResourceManager, *PostProcess, *ParticleSystem, inputSystem, *scriptManager);
    sceneManager->createScene("MainScene");
    sceneManager->setActiveScene("MainScene");

    // Opt-in fixed safety-net scene; reuse the editor's existing saved-scene loader.
    if (std::getenv("ENGINE_PHASE0_TEST_SCENE") != nullptr)
    {
        if (!sceneManager->loadScene("assets/scenes/phase0_test.json", "Phase0Test"))
            throw std::runtime_error("Phase 0 test scene load failed");
        se::log::info("Phase0Test", "Loaded assets/scenes/phase0_test.json");
    }

    imguiManager = std::make_unique<se::ImGuiManager>(
        seDevice, seRenderer.getSwapChainRenderPass(), *seWindow.getGLFWwindow(),
        seRenderer.getSwapChainImageCount(), *ResourceManager, *sceneManager,
        *PostProcess, *ParticleSystem, inputSystem, *scriptManager, performance);
    }
    catch (...)
    {
        // Body catch runs before member unwinding (a constructor function-try-block would not).
        closeFrameDisposals(seDevice, seRenderer, "vkDeviceWaitIdle(App construction failure)");
        throw;
    }
}

App::~App()
{
    if (seDevice.device() != VK_NULL_HANDLE)
        closeFrameDisposals(seDevice, seRenderer, "vkDeviceWaitIdle(App teardown)");
    if (sceneManager)
        sceneManager->destroyAllScenes();
}

void App::mainLoop()
{
    se::KeyboardMovementController cameraController{};
    std::string str;

    auto currentTime = std::chrono::high_resolution_clock::now();
    auto lastTime = currentTime;
    float totalTime = 0.f;
    int fps = 0;

    constexpr float fixedDt = 1.0f / 60.0f;
    float physicsAccumulator = 0.0f;
    const se::Scene* measuredScene = nullptr;
    se::SEGameObject::id_t measuredCreationCount = 0;

    // Opt-in smoke path for checking swapchain recreation after real window resizes.
    const bool resizeTestEnabled = std::getenv("ENGINE_PHASE2_TEST_RESIZE") != nullptr;
    const auto resizeTestStart = std::chrono::steady_clock::now();
    bool firstTestResizeIssued = false;
    bool secondTestResizeIssued = false;

    glfwSetInputMode(seWindow.getGLFWwindow(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    se::SEOffscreenRenderer* offscreenRenderer = seRenderer.getOffscreenRenderer();

    while (!seWindow.shouldClose())
    {
        glfwPollEvents();
        inputSystem.update();

        if (resizeTestEnabled)
        {
            const auto elapsed = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - resizeTestStart).count();
            if (!firstTestResizeIssued && elapsed >= 2.0f)
            {
                glfwSetWindowSize(seWindow.getGLFWwindow(), 1024, 768);
                se::log::info("ResizeTest", "requested glfwSetWindowSize 1024x768");
                firstTestResizeIssued = true;
            }
            if (!secondTestResizeIssued && elapsed >= 4.0f)
            {
                glfwSetWindowSize(seWindow.getGLFWwindow(), 1280, 720);
                se::log::info("ResizeTest", "requested glfwSetWindowSize 1280x720");
                secondTestResizeIssued = true;
            }
        }

        if (std::chrono::duration<float, std::chrono::seconds::period>(
                std::chrono::high_resolution_clock::now() - lastTime).count() > 1.f)
        {
            str.append(std::to_string(fps) + " FPS " + "( " + std::to_string(1.f / fps * 1000) + " ms )");
            seWindow.setTitle(str);
            str.clear();
            fps = 0;
            lastTime = std::chrono::high_resolution_clock::now();
        }

        const auto newTime = std::chrono::high_resolution_clock::now();
        const float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(
            newTime - currentTime).count();
        totalTime += frameTime;
        currentTime = newTime;

        se::TransformComponent transform = sceneManager->getCamera().getTransform();
        cameraController.moveInPlaneXZ(inputSystem, frameTime, transform);
        sceneManager->getCamera().setTransform(transform);
        sceneManager->getCamera().setViewYXZ();

        const float aspect = seRenderer.getAspectRatio();
        sceneManager->getCamera().setPerspectiveProjection(glm::radians(90.f), aspect, 0.1f, 100.f);

        auto* scene = sceneManager->getActiveScene();
        if (!scene)
        {
            se::log::error("App", "SCENE EMPTY!");
            return;
        }

        auto& gameObjects = scene->getGameObjects();
        scene->onUpdate(frameTime, *ResourceManager, inputSystem);

        const auto colliderCount = scene->getColliderCount();
        const auto dynamicBodyCount = scene->getDynamicBodyCount();
        const auto creationCount = scene->getObjectCreationCount();
        const auto& previousStats = performance.getStats();
        if (scene != measuredScene || creationCount != measuredCreationCount ||
            colliderCount != previousStats.colliderCount || dynamicBodyCount != previousStats.dynamicBodyCount)
        {
            performance.reset();
            measuredScene = scene;
            measuredCreationCount = creationCount;
        }
        performance.beginFrame(frameTime, colliderCount, dynamicBodyCount);

        physicsAccumulator += std::min(frameTime, 0.25f);
        while(physicsAccumulator >= fixedDt)
        {
            const auto physicsStart = std::chrono::steady_clock::now();
            scene->onFixedUpdate(fixedDt);
            performance.recordPhysicsStep(std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - physicsStart).count());
            physicsAccumulator -= fixedDt;
        }
        
        if (seWindow.wasWindowResized())
        {
            const auto extent = seWindow.getFramebufferExtent();
            if (extent.width != 0 && extent.height != 0)
            {
                offscreenRenderer->resize(extent.width, extent.height);
            }
        }

        const auto renderStart = std::chrono::steady_clock::now();
        auto frame = seRenderer.beginFrame();
        const auto renderBeginEnd = std::chrono::steady_clock::now();
        if (!frame)
        {
            ++fps;
            continue;
        }

        UniformBufferObject ubo{};
        ubo.proj = sceneManager->getCamera().getProjection();
        ubo.view = sceneManager->getCamera().getView();
        ubo.cameraPos = sceneManager->getCamera().getTransform().translation;
        seDevice.updateUniformBuffers(frame->frameIndex, ubo);

        const VkCommandBuffer commandBuffer = frame->commandBuffer;
        ParticleSystem->update(
            commandBuffer,
            static_cast<int>(frame->frameIndex),
            frameTime,
            totalTime);

        offscreenRenderer->beginRenderPass(commandBuffer, frame->frameIndex);
        PBR->renderCubeMap(commandBuffer, frame->frameIndex);
        PBR->renderGameObjects(commandBuffer, gameObjects, static_cast<int>(frame->frameIndex));
        ParticleSystem->render(
            commandBuffer,
            static_cast<int>(frame->frameIndex),
            sceneManager->getCamera().getProjection() * sceneManager->getCamera().getView());
        offscreenRenderer->endRenderPass(commandBuffer);

        PostProcess->updateFrameData(frame->frameIndex);
        PostProcess->setInput(
            frame->frameIndex,
            offscreenRenderer->getColorImageView(frame->frameIndex),
            offscreenRenderer->getDepthImageView(frame->frameIndex));

        seRenderer.beginSwapChainRenderPass(*frame);
        PostProcess->render(commandBuffer, frame->frameIndex);
        imguiManager->newFrame();
        imguiManager->render(commandBuffer);
        seRenderer.endSwapChainRenderPass(*frame);
        const auto renderRecordEnd = std::chrono::steady_clock::now();
        seRenderer.endFrame(*frame);
        const auto renderEnd = std::chrono::steady_clock::now();

        if (sceneManager->getActiveScene() == scene)
        {
            performance.recordRenderTimings(
                std::chrono::duration<double, std::milli>(renderBeginEnd - renderStart).count(),
                std::chrono::duration<double, std::milli>(renderRecordEnd - renderBeginEnd).count(),
                std::chrono::duration<double, std::milli>(renderEnd - renderRecordEnd).count());
        }
        ++fps;
    }

    closeFrameDisposals(seDevice, seRenderer, "vkDeviceWaitIdle(main loop shutdown)");
}
