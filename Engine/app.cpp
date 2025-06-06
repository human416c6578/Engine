#include "app.hpp"
#include "keyboard_movement_controller.hpp"
#include "se_material_system.hpp"

void App::mainLoop()
{
    
    se::SECamera camera{};
    auto viewerObject = se::SEGameObject::createGameObject();
    se::KeyboardMovementController cameraController{};
    static std::string str("");
    
    static auto currentTime = std::chrono::high_resolution_clock::now();

    static auto lastTime = std::chrono::high_resolution_clock::now();
    static int fps = 0;

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
        camera.setViewYXZ(viewerObject.getTransform().translation, viewerObject.getTransform().rotation);

        float aspect = seRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(90.f), aspect, 0.1f, 100.f);

        UniformBufferObject ubo{};
        ubo.proj = camera.getProjection();
        ubo.view = camera.getView();
        ubo.cameraPos = viewerObject.getTransform().translation;

        seDevice.updateUniformBuffers(ubo);
        
        for (auto& [_, mat] : *ResourceManager->getMaterials()) {
            mat->update();
        }
        

        if (auto commandBuffer = seRenderer.beginFrame())
        {
            seRenderer.beginSwapChainRenderPass(commandBuffer);

            PBR->renderGameObjects(commandBuffer, gameObjects, camera, viewerObject);
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

std::vector<glm::vec3> lightPositions = {
    glm::vec3(0.5f, 4.5f, 1.0f),
    glm::vec3(0.5f, 4.5f, 0.5f),
    glm::vec3(0.0f, 4.5f, 0.0f)};

std::vector<glm::vec3> lightColors = {
    glm::vec3(0.5f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.5f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.5f)};

void App::loadGameObjects()
{
    std::shared_ptr<se::SEMaterial> lightMat = ResourceManager->createMaterial("light_material");
    lightMat->setMetallic(0.2f);
    lightMat->setRoughness(0.9f);
    lightMat->setAO(0.5f);

    //std::shared_ptr<se::SEMesh> lightMesh = ResourceManager->loadMesh("models/test.obj");
	std::shared_ptr<se::SEMesh> lightMesh = ResourceManager->createSphere("light_mesh");

    for (int i = 0;i < 3;i++)
    {
        auto light = se::SEGameObject::createGameObject("light_"+std::to_string(i));
        light.setMesh(lightMesh);
		light.setTransform(se::TransformComponent{
            lightPositions[i], { 0.2f, 0.2f, 0.2f }, { .0f, .0f, .0f } });
        light.setColor(lightColors[i]);
		light.setMaterial(lightMat);
        gameObjects.push_back(std::move(light));
    }
   
	std::shared_ptr<se::SETexture> diffuse = ResourceManager->loadTexture("textures/Dark/texture_01.png");
	std::shared_ptr<se::SEMaterial> floorMat = ResourceManager->createMaterial("floor_material");

    floorMat->setMetallic(0.95f);
    floorMat->setRoughness(0.2f);
    floorMat->setAO(1.0f);
    floorMat->setDiffuseTexture(diffuse);

    
    //std::shared_ptr<se::SEMesh> floorMesh = std::make_unique<se::SEMesh>(seDevice, builder, floorMat);
    std::shared_ptr<se::SEMesh> cubeMesh = ResourceManager->createCube("floor_mesh");
	

    auto floor = se::SEGameObject::createGameObject("floor");
    floor.setMesh(cubeMesh);
	floor.setTransform(se::TransformComponent{
	{ .0f, 6.0f, .0f }, { 10.0f, 2.0f, 10.0f }, { .0f, .0f, .0f } });
    floor.setColor({ 1.f, 1.f, 1.f });
    floor.setMaterial(floorMat);

    gameObjects.push_back(std::move(floor));

    /*
    std::shared_ptr<se::SEMesh> sponzaMesh = ResourceManager->loadMesh("models/sponza/sponza.obj");

    auto sponza = se::SEGameObject::createGameObject();
    sponza.mesh = sponzaMesh;
    sponza.transform.translation = { .0f, 0.0f, .0f };
    sponza.transform.scale = { 0.01f, -0.01f, 0.01f };
    sponza.transform.rotation = { .0f, .0f, .0f };
    sponza.color = { 1.f, 1.f, 1.f };
    gameObjects.push_back(std::move(sponza));
    */

    /*
    std::shared_ptr<se::SEMesh> cerberusMesh = ResourceManager->loadMesh("models/cerberus/Cerberus_LP.FBX");
	std::shared_ptr<se::SEMaterial> cerberusMat = ResourceManager->createMaterial("cerberus_material");

	cerberusMat->setDiffuseTexture(ResourceManager->loadTexture("models/cerberus/Textures/Cerberus_A.tga"));
	cerberusMat->setNormalTexture(ResourceManager->loadTexture("models/cerberus/Textures/Cerberus_N.tga"));
	cerberusMat->setMetallicTexture(ResourceManager->loadTexture("models/cerberus/Textures/Cerberus_M.tga"));
	cerberusMat->setRoughnessTexture(ResourceManager->loadTexture("models/cerberus/Textures/Cerberus_R.tga"));

	cerberusMesh->setMaterial(cerberusMat);

    auto cerberus = se::SEGameObject::createGameObject();
    cerberus.mesh = cerberusMesh;
    cerberus.transform.translation = { .0f, 3.0f, .0f };
    cerberus.transform.scale = { 0.01f, 0.01f, 0.01f };
    cerberus.transform.rotation = { 3.14f/2.0f, .0f, .0f };
    cerberus.color = { 1.f, 1.f, 1.f };
    gameObjects.push_back(std::move(cerberus));

    */

}