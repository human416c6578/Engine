#include "app.hpp"
#include "keyboard_movement_controller.hpp"
#include "se_material_system.hpp"
#include "ExampleScript.hpp"
#include "TestScript.hpp"
#include "Snake.hpp"
#include "StressTest.hpp"

void App::mainLoop()
{
    se::KeyboardMovementController cameraController{};
    static std::string str("");

    static auto currentTime = std::chrono::high_resolution_clock::now();

    static auto lastTime = std::chrono::high_resolution_clock::now();
    static int fps = 0;

    glfwSetInputMode(seWindow.getGLFWwindow(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

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
        float frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
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

        if (auto commandBuffer = seRenderer.beginFrame())
        {
            seRenderer.beginSwapChainRenderPass(commandBuffer);

            PBR->renderGameObjects(commandBuffer, gameObjects, seRenderer.getFrameIndex());
            PBR->renderCubeMap(commandBuffer);

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