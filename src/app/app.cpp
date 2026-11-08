#include "engine/app/app.hpp"
#include "engine/vulkan/se_vk_result.hpp"
#include <string>
#include <memory>
#include <iostream>
#include <exception>
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
#include "engine/scripting/StressTest.hpp"
#include "engine/scripting/se_script_manager.hpp"

App::App(se::Logger& log) : logger{log}
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
    scriptManager->registerScript<se::StressScript>("StressScript");

    sceneManager = std::make_unique<se::SceneManager>(
        *ResourceManager, *PostProcess, *ParticleSystem, inputSystem, *scriptManager, logger);
    sceneManager->createScene("MainScene");
    sceneManager->setActiveScene("MainScene");

    imguiManager = std::make_unique<se::ImGuiManager>(
        seDevice, seRenderer.getSwapChainRenderPass(), *seWindow.getGLFWwindow(),
        seRenderer.getSwapChainImageCount(), *ResourceManager, *sceneManager,
        *PostProcess, *ParticleSystem, inputSystem, *scriptManager);
}

App::~App()
{
    if (seDevice.device() != VK_NULL_HANDLE)
    {
        const VkResult idleResult = vkDeviceWaitIdle(seDevice.device());
        seDevice.logResult(idleResult, "vkDeviceWaitIdle(App teardown)");
    }
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

        if (resizeTestEnabled)
        {
            const auto elapsed = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - resizeTestStart).count();
            if (!firstTestResizeIssued && elapsed >= 2.0f)
            {
                glfwSetWindowSize(seWindow.getGLFWwindow(), 1024, 768);
                logger.log(se::LogLevel::Info, "ResizeTest", "requested glfwSetWindowSize 1024x768");
                firstTestResizeIssued = true;
            }
            if (!secondTestResizeIssued && elapsed >= 4.0f)
            {
                glfwSetWindowSize(seWindow.getGLFWwindow(), 1280, 720);
                logger.log(se::LogLevel::Info, "ResizeTest", "requested glfwSetWindowSize 1280x720");
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
            logger.log(se::LogLevel::Error, "App", "SCENE EMPTY!");
            return;
        }

        auto& gameObjects = scene->getGameObjects();
        scene->onUpdate(frameTime, *ResourceManager, inputSystem);

        if (seWindow.wasWindowResized())
        {
            const auto extent = seWindow.getFramebufferExtent();
            if (extent.width != 0 && extent.height != 0)
            {
                offscreenRenderer->resize(extent.width, extent.height);
            }
        }

        auto frame = seRenderer.beginFrame();
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
        PBR->renderCubeMap(commandBuffer);
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
        seRenderer.endFrame(*frame);
        ++fps;
    }

    se::checkVkResult(vkDeviceWaitIdle(seDevice.device()), "vkDeviceWaitIdle(main loop shutdown)");
}
