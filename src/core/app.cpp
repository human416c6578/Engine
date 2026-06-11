#include "engine/core/app.hpp"
#include "engine/input/keyboard_movement_controller.hpp"
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

    //auto snow = ParticleSystem->makeSnow(glm::vec3(0.0f, -5.0f, 0.0f), glm::vec3(20.0f, 2.0f, 20.0f));
    auto rain = ParticleSystem->makeRain(glm::vec3(0.0f, -5.0f, 0.0f), glm::vec3(5.0f, 2.0f, 5.0f));
    //auto& emitter = ParticleSystem->getEmitter(rain.emitter);
    //auto blackhole = ParticleSystem->makeBlackHole(glm::vec3(0.0f, 0.0f, 0.0f), 10.0f);

    while (!seWindow.shouldClose())
    {
        glfwPollEvents();

        if (std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - lastTime).count() > 1.f)
        {
            str.append(std::to_string(fps) + " FPS " + "( " + std::to_string(1.f / fps * 1000) + " ms )");
            seWindow.setTitle(str);
            str.clear();

            fps = 0;
            lastTime = std::chrono::high_resolution_clock::now();
        }

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();

        totalTime += frameTime;
        currentTime = newTime;
        se::TransformComponent transform = sceneManager->getCamera().getTransform();
        cameraController.moveInPlaneXZ(seWindow.getGLFWwindow(), frameTime, transform);
        sceneManager->getCamera().setTransform(transform);
        sceneManager->getCamera().setViewYXZ();

        float aspect = seRenderer.getAspectRatio();
        sceneManager->getCamera().setPerspectiveProjection(glm::radians(90.f), aspect, 0.1f, 100.f);

        UniformBufferObject ubo{};
        ubo.proj = sceneManager->getCamera().getProjection();
        ubo.view = sceneManager->getCamera().getView();
        ubo.cameraPos = sceneManager->getCamera().getTransform().translation;

        seDevice.updateUniformBuffers(ubo);

        auto* scene = sceneManager->getActiveScene();
        if (!scene) {
            printf("[ERROR]: SCENE EMPTY!");
            return;
        }

        auto& gameObjects = scene->getGameObjects();
        scene->onUpdate(frameTime);

        if(seWindow.wasWindowResized())
        {
            auto extent = seWindow.getFramebufferExtent();
            offscreenRenderer->resize(extent.width, extent.height);
        }
        int frame = 0;
        if (auto commandBuffer = offscreenRenderer->beginFrame())
        {
            offscreenRenderer->beginRenderPass(commandBuffer);

            frame = offscreenRenderer->getCurrentFrame();

            ParticleSystem->update(commandBuffer, frame, frameTime, totalTime);

            PBR->renderCubeMap(commandBuffer);

            PBR->renderGameObjects(commandBuffer, gameObjects, frame);

            ParticleSystem->render(
                commandBuffer,
                frame,
                sceneManager->getCamera().getProjection() *
                sceneManager->getCamera().getView()
            );

            offscreenRenderer->endRenderPass(commandBuffer);
            offscreenRenderer->endFrame();
        }

        if (auto commandBuffer = seRenderer.beginFrame())
        {
            seRenderer.beginSwapChainRenderPass(commandBuffer);

            //PBR->renderGameObjects(commandBuffer, gameObjects, seRenderer.getFrameIndex());

            PostProcess->setInput(
                 offscreenRenderer->getColorImageView(frame),
                 offscreenRenderer->getDepthImageView(frame)
            );
            PostProcess->render(commandBuffer);

            imguiManager.newFrame();
            imguiManager.render(commandBuffer);

            seRenderer.endSwapChainRenderPass(commandBuffer);
            seRenderer.endFrame();
        }
        fps++;
    }

    vkDeviceWaitIdle(seDevice.device());
    imguiManager.cleanup();

}
