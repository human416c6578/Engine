#include "app.hpp"
#include "keyboard_movement_controller.hpp"

void App::mainLoop()
{
    
    se::SECamera camera{};
    auto viewerObject = se::SEGameObject::createGameObject();
    se::KeyboardMovementController cameraController{};
    static std::string str("");
    
    static auto currentTime = std::chrono::high_resolution_clock::now();

    static auto lastTime = std::chrono::high_resolution_clock::now();
    static int fps = 0;

    se::PBR PBR{ seDevice, seRenderer.getSwapChainRenderPass(), seCubemap };

    loadGameObjects(PBR.getMaterialDescriptorSetLayout());

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
        camera.setPerspectiveProjection(glm::radians(90.f), aspect, 0.1f, 100.f);

        UniformBufferObject ubo{};
        ubo.proj = camera.getProjection();
        ubo.view = camera.getView();
        ubo.cameraPos = viewerObject.transform.translation;

        seDevice.updateUniformBuffers(ubo);

        if (auto commandBuffer = seRenderer.beginFrame())
        {
            seRenderer.beginSwapChainRenderPass(commandBuffer);

            PBR.renderGameObjects(commandBuffer, gameObjects, camera, viewerObject);
            PBR.renderCubeMap(commandBuffer);

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
        {{-.5f, -.5f, -.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-.5f, .5f, .5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-.5f, -.5f, .5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-.5f, .5f, -.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

        // right face
        {{.5f, -.5f, -.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, .5f, .5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{.5f, -.5f, .5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, .5f, -.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

        // top face
        {{-.5f, -.5f, -.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, -.5f, .5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-.5f, -.5f, .5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, -.5f, -.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},

        // bottom face
        {{-.5f, .5f, -.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, .5f, .5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-.5f, .5f, .5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, .5f, -.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},

        // nose face
        {{-.5f, -.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{.5f, .5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-.5f, .5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{.5f, -.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},

        // tail face
        {{-.5f, -.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{.5f, .5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-.5f, .5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{.5f, -.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
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

static se::SESubMesh::Builder createSphereModel(float radius, int sectorCount, int stackCount, glm::vec3 offset)
{
    se::SESubMesh::Builder modelBuilder{};

    // Variables for spherical coordinates
    float sectorStep = 2 * glm::pi<float>() / sectorCount;  // Angle between each sector
    float stackStep = glm::pi<float>() / stackCount;        // Angle between each stack

    // Generate vertices
    for (int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = glm::pi<float>() / 2 - i * stackStep; // Angle from the top (-π/2) to the bottom (π/2)
        float xy = radius * cos(stackAngle);                     // Radius at this stack level
        float z = radius * sin(stackAngle);                      // Z position at this stack level

        for (int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * sectorStep; // Angle around the Y-axis (0 to 2π)

            // Calculate vertex position in Cartesian coordinates
            glm::vec3 position = glm::vec3(
                xy * cos(sectorAngle), // X position
                xy * sin(sectorAngle), // Y position
                z                      // Z position
            );

            // Add the vertex with position and normal (normalized)
            modelBuilder.vertices.push_back({
                position + offset,  // Apply the offset to each vertex
                glm::normalize(position), // The normal is just the normalized position vector
                glm::vec2((float)j / sectorCount, (float)i / stackCount)  // Texture coordinates
                });
        }
    }

    // Generate indices
    for (int i = 0; i < stackCount; ++i)
    {
        int k1 = i * (sectorCount + 1);     // First vertex of the current stack
        int k2 = k1 + sectorCount + 1;      // First vertex of the next stack

        for (int j = 0; j < sectorCount; ++j)
        {
            if (i != 0) // Skip the first stack (the top of the sphere)
            {
                modelBuilder.indices.push_back(k1 + j);
                modelBuilder.indices.push_back(k2 + j);
                modelBuilder.indices.push_back(k1 + j + 1);
            }

            if (i != (stackCount - 1)) // Skip the last stack (the bottom of the sphere)
            {
                modelBuilder.indices.push_back(k1 + j + 1);
                modelBuilder.indices.push_back(k2 + j);
                modelBuilder.indices.push_back(k2 + j + 1);
            }
        }
    }

    return modelBuilder;
}


std::vector<glm::vec3> lightPositions = {
    glm::vec3(0.5f, -1.0f, 0.0f),
    glm::vec3(0.5f, -1.0f, 0.5f),
    glm::vec3(0.0f, -1.0f, 0.0f)};

std::vector<glm::vec3> lightColors = {
    glm::vec3(0.5f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.5f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.5f)};

void App::loadGameObjects(VkDescriptorSetLayout descriptorSetLayout)
{
    /*
    std::shared_ptr<se::SEMesh> sponzaMesh = std::make_unique<se::SEMesh>(seDevice, "models/sponza/sponza.obj", seRenderer.getSwapChainRenderPass(), descriptorSetLayout);
   
    auto scene = se::SEGameObject::createGameObject();
    scene.mesh = sponzaMesh;
    scene.transform.translation = { 1.0f, 7.5f, .0f };
    scene.transform.scale = { .01f, -.01f, .01f };
    scene.transform.rotation = { .0f, .0f, .0f };
    scene.color = { 1.f, 1.f, 1.f };
    gameObjects.push_back(std::move(scene));
    */
    
    int nrRows = 5;
    int nrColumns = 5;
    float spacing = 2.5f; // Distance between spheres
    
    
    for (int row = 0; row < nrRows; ++row)
    {
        for (int col = 0; col < nrColumns; ++col)
        {
            // Dynamically create a new material for each sphere with varying metallic and roughness
            std::shared_ptr<se::SEMaterial> material = std::make_shared<se::SEMaterial>(
                seDevice, descriptorSetLayout, VK_SAMPLE_COUNT_1_BIT,
                (float)row / (float)nrRows, // Set metallic based on row
                glm::clamp((float)col / (float)nrColumns, 0.05f, 1.0f) // Set roughness based on column, clamped to avoid 0.0 roughness
            );
            
            // Create a new sphere mesh using the createSphereModel function
            float radius = 1.0f;  // You can adjust the radius as needed
            int sectorCount = 36; // Number of sectors (longitude divisions)
            int stackCount = 18;  // Number of stacks (latitude divisions)

            // Create the sphere model dynamically
            auto sphereModel = createSphereModel(radius, sectorCount, stackCount, glm::vec3(0.0f, 0.0f, 0.0f));
            //auto sphereModel = createCubeModel(glm::vec3(0.0f, 0.0f, 0.0f));
            // Create the mesh for the sphere
            std::shared_ptr<se::SEMesh> sphereMesh = std::make_shared<se::SEMesh>(
                seDevice, sphereModel, material);

            // Create a new game object for this sphere
            auto sphere = se::SEGameObject::createGameObject();
            sphere.mesh = sphereMesh;

            // Set the transformation for the sphere (position, scale, rotation)
            sphere.transform.translation = glm::vec3(
                (float)(col - (nrColumns / 2)) * spacing,
                (float)(row - (nrRows / 2)) * spacing,
                -2.0f
            );
            sphere.transform.scale = glm::vec3(0.5f, 0.5f, 0.5f); // Small scale for each sphere
            sphere.transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f); // No rotation

            // Set the color to white or any desired color for this sphere
            sphere.color = glm::vec3(1.0f, 1.0f, 1.0f);
            
            // Add this sphere to the gameObjects collection
            gameObjects.push_back(std::move(sphere));
        }
    }
    
    se::SESubMesh::Builder builder = createCubeModel({0, 0, 0});
   
    
    std::shared_ptr<se::SETexture> diffuse = std::make_unique<se::SETexture>(seDevice, "textures/Dark/texture_01.png");
   
    std::shared_ptr<se::SEMaterial> material = std::make_unique<se::SEMaterial>(seDevice, descriptorSetLayout, VK_SAMPLE_COUNT_1_BIT, 0.00, 0.95, 1.0, diffuse);
    
    std::shared_ptr<se::SEMesh> floorMesh = std::make_unique<se::SEMesh>(seDevice, builder, material);

    auto floor = se::SEGameObject::createGameObject();
    floor.mesh = floorMesh;
    floor.transform.translation = {.0f, 6.0f, .0f};
    floor.transform.scale = {10.0f, 0.5f, 10.0f};
    floor.transform.rotation = {.0f, .0f, .0f};
    floor.color = {0.f, 0.f, 0.f};
    gameObjects.push_back(std::move(floor));

    
}