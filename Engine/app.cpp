#include "app.hpp"
#include "keyboard_movement_controller.hpp"

void App::mainLoop()
{
    se::SimpleRenderSystem simpleRenderSystem{seDevice, seRenderer.getSwapChainRenderPass()};
    se::SECamera camera{};
    auto viewerObject = se::SEGameObject::createGameObject();
    se::KeyboardMovementController cameraController{};
    static std::string str("");

    static auto currentTime = std::chrono::high_resolution_clock::now();

    static auto lastTime = std::chrono::high_resolution_clock::now();
    static int fps = 0;
    loadCubemap();
    loadGameObjects();

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
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspect = seRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(90.f), aspect, 0.01f, 1000.f);

        UniformBufferObject ubo{};
        ubo.proj = camera.getProjection();
        ubo.view = camera.getView();
        ubo.cameraPos = viewerObject.transform.translation;

        seDevice.updateUniformBuffers(ubo);

        if (auto commandBuffer = seRenderer.beginFrame())
        {
            seRenderer.beginSwapChainRenderPass(commandBuffer);

            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera, viewerObject);

            seRenderer.endSwapChainRenderPass(commandBuffer);
            seRenderer.endFrame();
        }

        fps++;
    }

    vkDeviceWaitIdle(seDevice.device());
}

static se::SESubMesh::Builder createCubeModel(glm::vec3 offset)
{
    se::SESubMesh::Builder modelBuilder{};

    // Define the vertices for the cube. Each face has 4 vertices.
    modelBuilder.vertices = {
        // left face
        {{-.5f, -.5f, -.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-.5f, .5f, .5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-.5f, -.5f, .5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-.5f, .5f, -.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},

        // right face
        {{.5f, -.5f, -.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, .5f, .5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{.5f, -.5f, .5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, .5f, -.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

        // top face
        {{-.5f, -.5f, -.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, -.5f, .5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-.5f, -.5f, .5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, -.5f, -.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},

        // bottom face
        {{-.5f, .5f, -.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, .5f, .5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-.5f, .5f, .5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, .5f, -.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},

        // nose face
        {{-.5f, -.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{.5f, .5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-.5f, .5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{.5f, -.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},

        // tail face
        {{-.5f, -.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, .5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-.5f, .5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, -.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    };

    // Apply offset to positions
    for (auto &v : modelBuilder.vertices)
    {
        v.position += offset;
    }

    // Define the indices for drawing the cube with triangles
    modelBuilder.indices = {
        0, 1, 2, 0, 3, 1,       // left face
        4, 5, 6, 4, 7, 5,       // right face
        8, 9, 10, 8, 11, 9,     // top face
        12, 13, 14, 12, 15, 13, // bottom face
        16, 17, 18, 16, 19, 17, // nose face
        20, 21, 22, 20, 23, 21  // tail face
    };

    return modelBuilder;
}

void App::loadCubemap()
{
    seCubemap = std::make_unique<se::SECubemap>(seDevice, seRenderer, "hdr/snowy_forest_4k.hdr");
}

std::vector<glm::vec3> lightPositions = {
    glm::vec3(0.5f, -1.0f, 0.0f),
    glm::vec3(0.5f, -1.0f, 0.5f),
    glm::vec3(0.0f, -1.0f, 0.0f)};

std::vector<glm::vec3> lightColors = {
    glm::vec3(0.5f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.5f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.5f)};

void App::loadGameObjects()
{
    std::shared_ptr<se::SETexture> diffuse = std::make_unique<se::SETexture>(seDevice, "textures/patched-brickwork_albedo.png");
    std::shared_ptr<se::SETexture> normal = std::make_unique<se::SETexture>(seDevice, "textures/patched-brickwork_normal-dx.png");
    std::shared_ptr<se::SETexture> metallic = std::make_unique<se::SETexture>(seDevice, "textures/patched-brickwork_metallic.png");
    std::shared_ptr<se::SETexture> roughness = std::make_unique<se::SETexture>(seDevice, "textures/patched-brickwork_roughness.png");
    std::shared_ptr<se::SETexture> ao = std::make_unique<se::SETexture>(seDevice, "textures/patched-brickwork_ao.png");

    std::shared_ptr<se::SEMaterial> material = std::make_unique<se::SEMaterial>(seDevice, seRenderer.getSwapChainRenderPass(), "shaders/matvert.spv", "shaders/matfrag.spv", VK_SAMPLE_COUNT_1_BIT, 0.1, 0.7, 1.0, diffuse, normal, metallic, roughness, ao);

    std::shared_ptr<se::SEMaterial> material2 = std::make_unique<se::SEMaterial>(seDevice, seRenderer.getSwapChainRenderPass(), "shaders/matvert.spv", "shaders/matfrag.spv", VK_SAMPLE_COUNT_1_BIT, 1.0, 0.01, 1.0);

    se::SESubMesh::Builder builder = createCubeModel({0, 0, 0});
    std::shared_ptr<se::SEMesh> floorMesh = std::make_unique<se::SEMesh>(seDevice, builder, material);
    /*
    std::shared_ptr<se::SEMesh> sponzaMesh = std::make_unique<se::SEMesh>(seDevice, "models/sponza/sponza.obj", seRenderer.getSwapChainRenderPass());
   
    
    auto scene = se::SEGameObject::createGameObject();
    scene.mesh = sponzaMesh;
    scene.transform.translation = { 1.0f, 2.5f, .0f };
    scene.transform.scale = { .01f, -.01f, .01f };
    scene.transform.rotation = { .0f, .0f, .0f };
    scene.color = { 1.f, 1.f, 1.f };
    gameObjects.push_back(std::move(scene));
    */

    auto floor = se::SEGameObject::createGameObject();
    floor.mesh = floorMesh;
    floor.transform.translation = {1.0f, 2.5f, .0f};
    floor.transform.scale = {2.5f, 0.5f, 2.5f};
    floor.transform.rotation = {.0f, .0f, .0f};
    floor.color = {.2f, .2f, .2f};
    gameObjects.push_back(std::move(floor));
}