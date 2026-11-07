#include "engine/app/app.hpp"
#include <cstdio>
#include "engine/platform/keyboard_movement_controller.hpp"
#include "engine/renderer/se_material_system.hpp"
#include "engine/scripting/ExampleScript.hpp"
#include "engine/scripting/TestScript.hpp"
#include "engine/scripting/Snake.hpp"
#include "engine/scripting/StressTest.hpp"

void App::mainLoop()
{
    se::KeyboardMovementController cameraController{};
    static std::string str("");

    static auto currentTime = std::chrono::high_resolution_clock::now();
    static auto lastTime = std::chrono::high_resolution_clock::now();
    static float totalTime = 0.f;
    static int fps = 0;

    glfwSetInputMode(seWindow.getGLFWwindow(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    se::SEOffscreenRenderer* offscreenRenderer = seRenderer.getOffscreenRenderer();

    while (!seWindow.shouldClose())
    {
        glfwPollEvents();

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
        cameraController.moveInPlaneXZ(seWindow.getGLFWwindow(), frameTime, transform);
        sceneManager->getCamera().setTransform(transform);
        sceneManager->getCamera().setViewYXZ();

        const float aspect = seRenderer.getAspectRatio();
        sceneManager->getCamera().setPerspectiveProjection(glm::radians(90.f), aspect, 0.1f, 100.f);

        auto* scene = sceneManager->getActiveScene();
        if (!scene)
        {
            std::fprintf(stderr, "[ERROR]: SCENE EMPTY!\n");
            return;
        }

        auto& gameObjects = scene->getGameObjects();
        scene->onUpdate(frameTime);

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
        imguiManager.newFrame();
        imguiManager.render(commandBuffer);
        seRenderer.endSwapChainRenderPass(*frame);
        seRenderer.endFrame(*frame);
        ++fps;
    }

    vkDeviceWaitIdle(seDevice.device());
    imguiManager.cleanup();
}
