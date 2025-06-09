#include "app.hpp"
#include "keyboard_movement_controller.hpp"
#include "se_material_system.hpp"
#include "ExampleScript.hpp"
#include "TestScript.hpp"

void App::mainLoop()
{

    se::SECamera camera{};
    auto viewerObject = se::SEGameObject::createGameObject();
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

        cameraController.moveInPlaneXZ(seWindow.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.getTransform().translation, viewerObject.getTransform().rotation);

        float aspect = seRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(90.f), aspect, 0.1f, 100.f);

        UniformBufferObject ubo{};
        ubo.proj = camera.getProjection();
        ubo.view = camera.getView();
        ubo.cameraPos = viewerObject.getTransform().translation;

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

            PBR->renderGameObjects(commandBuffer, gameObjects, camera, viewerObject, seRenderer.getFrameIndex());
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