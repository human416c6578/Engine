#include "imgui_manager.hpp"

void se::ImGuiManager::renderSceneHierarchy()
{
    static int emptyCount = 0;
	static int cubeCount = 0;
    static int sphereCount = 0;
    bool itemHovered = false;

    if (!showSceneHierarchy) return;

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    // Fixed to left, above Asset Browser, 300px wide
    float assetBrowserHeight = 200.0f;
    ImGui::SetNextWindowPos(workPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, workSize.y - assetBrowserHeight), ImGuiCond_Always);

    ImGui::Begin("Scene Hierarchy", &showSceneHierarchy,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Game Objects (%d)", gameobjects->size());
    ImGui::Separator();

    for (int i = 0; i < gameobjects->size(); i++)
    {
        auto& gameObject = gameobjects->at(i);
        ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
        bool isSelected = (selectedGameObjectIndex == i);

        if (ImGui::Selectable((gameObject.getName() + "##" +std::to_string(gameObject.getId())).c_str(), isSelected, flags))
        {
            selectedGameObjectIndex = i;
            selectedAssetName = "";
            selectedAssetCategory = "";
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            itemHovered = true;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                gameobjects->erase(gameobjects->begin() + i);
                if (selectedGameObjectIndex == i) selectedGameObjectIndex = -1;
                else if (selectedGameObjectIndex > i) selectedGameObjectIndex--;
            }
            ImGui::EndPopup();
        }
    }


    if (!itemHovered && ImGui::BeginPopupContextWindow()) {
        if (ImGui::MenuItem("Create Empty")) {
            gameobjects->emplace_back(std::move(SEGameObject::createGameObject("Empty_" + std::to_string(emptyCount++))));
        }
        if (ImGui::MenuItem("Create Cube")) {
            SEGameObject go = SEGameObject::createGameObject("Cube_" + std::to_string(cubeCount++));
            go.setMesh(resourceManager->createCube("CubeMesh_" + std::to_string(cubeCount)));
			go.setMaterial(resourceManager->createMaterial("CubeMaterial_" + std::to_string(cubeCount)));
			go.setColor({ 1.0f, 1.0f, 1.0f });
            gameobjects->emplace_back(std::move(go));
        }
        if (ImGui::MenuItem("Create Sphere")) {
            SEGameObject go = SEGameObject::createGameObject("Sphere_" + std::to_string(sphereCount++));
            go.setMesh(resourceManager->createSphere("SphereMesh_" + std::to_string(sphereCount)));
            go.setMaterial(resourceManager->createMaterial("SphereMaterial_" + std::to_string(sphereCount)));
            go.setColor({ 1.0f, 1.0f, 1.0f });
            gameobjects->emplace_back(std::move(go));
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}



void se::ImGuiManager::renderAssetBrowser()
{
    if (!showAssetBrowser) return;

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    // Fixed to bottom left, 300px wide, 200px tall
    ImGui::SetNextWindowPos(ImVec2(workPos.x, workPos.y + workSize.y - 200), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);

    ImGui::Begin("Asset Browser", &showAssetBrowser,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Asset Categories");
    ImGui::Separator();

    for (const auto& category : assetCategories)
    {
        ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
        bool isSelected = (selectedAssetCategory == category);

        if (ImGui::Selectable(category.c_str(), isSelected, flags))
        {
            selectedAssetCategory = category;
            selectedAssetName = "";
            selectedGameObjectIndex = -1;
        }
    }
    
    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::MenuItem("Create Material")) {
            std::string name = "New Material " + std::to_string(resourceManager->getMaterials()->size());
            resourceManager->createMaterial(name);
        }
        if (ImGui::MenuItem("Load Mesh")) 
        {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModel", "Choose File", ".obj,.fbx,.*", config);
        }
        if (ImGui::MenuItem("Load Texture")) {
            IGFD::FileDialogConfig config;
            config.path = ".";

            ImGuiFileDialog::Instance()->OpenDialog("ChooseTexture", "Choose File", ".png,.jpg,.tga,.*", config);
        }
        ImGui::EndPopup();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseModel")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

            resourceManager->loadMesh(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseTexture")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

            resourceManager->loadTexture(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
    }


    

    ImGui::End();
}

void se::ImGuiManager::renderAssetViewer()
{
    if (!showAssetViewer) return;

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    float left = workPos.x + 300.0f;
    float right = workPos.x + workSize.x - 400.0f;
    ImVec2 viewerPos(left, workPos.y + workSize.y - 200.0f);
    ImVec2 viewerSize(right - left, 200.0f);

    ImGui::SetNextWindowPos(viewerPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(viewerSize, ImGuiCond_Always);

    ImGui::Begin("Asset Viewer", &showAssetViewer,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Category: %s", selectedAssetCategory.c_str());
    ImGui::Separator();

    if (selectedAssetCategory == "Meshes")
    {
        auto meshes = resourceManager->getMeshes();
        for (const auto& [key, mesh] : *meshes)
        {
            std::string name = mesh->getName().empty() ? key : mesh->getName();

            ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
            bool isSelected = (selectedAssetName == key);

            if (ImGui::Selectable(name.c_str(), isSelected, flags))
            {
                selectedAssetName = key;
                selectedGameObjectIndex = -1;
            }
        }
    }
    else if (selectedAssetCategory == "Materials")
    {
        auto materials = resourceManager->getMaterials();
        for (const auto& [key, material] : *materials)
        {
            std::string name = material->getName().empty() ? key : material->getName();
            ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
            bool isSelected = (selectedAssetName == key);

            if (ImGui::Selectable(name.c_str(), isSelected, flags))
            {
                selectedAssetName = key;
                selectedGameObjectIndex = -1;
            }
        }
    }
    else if (selectedAssetCategory == "Textures")
    {
        auto textures = resourceManager->getTextures();
        for (const auto& [key, texture] : *textures)
        {
            std::string name = texture->getName().empty() ? key : texture->getName();
            ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
            bool isSelected = (selectedAssetName == key);

            if (ImGui::Selectable(name.c_str(), isSelected, flags))
            {
                selectedAssetName = key;
                selectedGameObjectIndex = -1;
            }
        }
    }

    if (selectedAssetCategory == "Textures" && !selectedAssetName.empty())
    {
        ImGui::Separator();
        ImGui::Text("Texture Preview");
        // Add texture preview here if needed
    }

    ImGui::End();
}

void se::ImGuiManager::renderPropertiesPanel()
{
    if (!showProperties) return;

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    // Fixed to right, full height, 400px wide
    ImGui::SetNextWindowPos(ImVec2(workPos.x + workSize.x - 400, workPos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, workSize.y), ImGuiCond_Always);

    ImGui::Begin("Properties", &showProperties,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (selectedGameObjectIndex >= 0 && selectedGameObjectIndex < gameobjects->size())
    {
        renderGameObjectProperties();
    }
    else if (!selectedAssetName.empty())
    {
        if (selectedAssetCategory == "Meshes")
        {
            renderMeshProperties();
        }
        else if (selectedAssetCategory == "Materials")
        {
            renderMaterialProperties();
        }
        else if (selectedAssetCategory == "Textures")
        {
            renderTextureProperties();
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "Select a game object or asset to view properties");
    }

    ImGui::End();
}

void se::ImGuiManager::renderGameObjectProperties()
{
    auto& gameObject = (*gameobjects)[selectedGameObjectIndex];
    static char nameBuffer[128] = { 0 };

    strcpy_s(nameBuffer, sizeof(nameBuffer), gameObject.getName().c_str());

    ImGui::Text("GameObject: ", gameObject.getName().c_str());
    ImGui::SameLine();

    if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer))) {
        gameObject.setName(nameBuffer);
    }

    std::string name = gameObject.getName().empty() ?
        ("GameObject_" + std::to_string(selectedGameObjectIndex)) : gameObject.getName();

    ImGui::Text("GameObject: %s", name.c_str());
    ImGui::Separator();

    renderTransformComponent(gameObject);
    renderMeshComponent(gameObject);
	renderMaterialComponent(gameObject);
}

void se::ImGuiManager::renderTransformComponent(se::SEGameObject& gameObject)
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto transform = gameObject.getTransform();

        float position[3] = { transform.translation.x, transform.translation.y, transform.translation.z };
        float rotation[3] = { transform.rotation.x, transform.rotation.y, transform.rotation.z };
        float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };

        float color[3] = { gameObject.getColor().r, gameObject.getColor().g, gameObject.getColor().b };

        bool changed = false;

        if (ImGui::DragFloat3("Position", position, 0.1f)) {
            changed = true;
        }
        if (ImGui::DragFloat3("Rotation", rotation, 0.1f)) {
            changed = true;
        }
        if (ImGui::DragFloat3("Scale", scale, 0.1f)) {
            changed = true;
        }

        if (changed) {
            TransformComponent newTransform = transform;
            newTransform.translation = { position[0], position[1], position[2] };
            newTransform.rotation = { rotation[0], rotation[1], rotation[2] };
            newTransform.scale = { scale[0], scale[1], scale[2] };
            gameObject.setTransform(newTransform);
        }


        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::DragFloat3("Color", color, 0.01f))
        {
            gameObject.setColor({
                std::clamp(color[0], 0.0f, 1.0f),
                std::clamp(color[1], 0.0f, 1.0f),
                std::clamp(color[2], 0.0f, 1.0f)
            });
        }

        ImGui::SameLine();
        if (ImGui::ColorEdit3("Color", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
        {
            gameObject.setColor({ color[0], color[1], color[2] });
        }
    }
}

void se::ImGuiManager::renderMeshComponent(se::SEGameObject& gameObject)
{
    if (ImGui::CollapsingHeader("Mesh Renderer"))
    {
        // Display current mesh
        if (gameObject.getMesh())
        {
            ImGui::Text("Mesh: %s", gameObject.getMesh()->getName().c_str());

            if (ImGui::Button("Change Mesh"))
            {
                ImGui::OpenPopup("Select Mesh");
            }
        }
        else
        {
            ImGui::Text("No mesh assigned");
            if (ImGui::Button("Assign Mesh"))
            {
                ImGui::OpenPopup("Select Mesh");
            }
        }

        // Mesh selection popup
        if (ImGui::BeginPopup("Select Mesh"))
        {
            auto meshes = resourceManager->getMeshes();
            for (const auto& [key, mesh] : *meshes)
            {
				std::string name = mesh->getName().empty() ? key : mesh->getName();
                if (ImGui::Selectable(name.c_str()))
                {
                    gameObject.setMesh(mesh);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }
}

void se::ImGuiManager::renderMaterialComponent(se::SEGameObject& gameObject)
{
    if (ImGui::CollapsingHeader("Material"))
    {

        // Display current material
        if (gameObject.getMaterial())
        {
            auto material = gameObject.getMaterial();
            ImGui::Text("Material: %s", material->getName().c_str());

            if (ImGui::Button("Change Material"))
            {
                ImGui::OpenPopup("Select Material");
            }

            ImGui::Separator();
            renderMaterialEditor(material);
        }
        else
        {
            ImGui::Text("No material assigned");
            if (ImGui::Button("Assign Material"))
            {
                ImGui::OpenPopup("Select Material");
            }
        }

        // Material selection popup
        if (ImGui::BeginPopup("Select Material"))
        {
            auto materials = resourceManager->getMaterials();
            for (const auto& [key, material] : *materials)
            {
                std::string name = material->getName().empty() ? key : material->getName();
                if (ImGui::Selectable(name.c_str()))
                {
                    gameObject.setMaterial(material);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }
    else
    {
        ImGui::Text("No mesh assigned to object");
    }

}

void se::ImGuiManager::renderMeshProperties()
{
    static char nameBuffer[128] = { 0 };

    auto meshes = resourceManager->getMeshes();
    auto it = meshes->find(selectedAssetName);
    if (it != meshes->end())
    {
        auto mesh = it->second;

        strcpy_s(nameBuffer, sizeof(nameBuffer), mesh->getName().c_str());

        ImGui::Text("Mesh: ", mesh->getName().c_str());
        ImGui::SameLine();

        if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer))) {
            mesh->setName(nameBuffer);
        }

        ImGui::Text("GUID: %s", mesh->getGUID().c_str());
        ImGui::Separator();

        // Mesh-specific properties
        ImGui::Text("Submeshes: %d", mesh->getSubMeshCount());
        ImGui::Text("Vertices: %d", mesh->getVerticesCount());
        ImGui::Text("Triangles: %d", mesh->getIndicesCount());

        /*
        if (mesh->hasMaterial())
        {
            ImGui::Text("Assigned Material: %s", mesh->getMaterial()->getName().c_str());

            ImGui::Separator();
            if (ImGui::Button("Change Material"))
            {
                ImGui::OpenPopup("Select Mesh Material");
            }

            // Material selection popup for mesh
            if (ImGui::BeginPopup("Select Mesh Material"))
            {
                auto materials = resourceManager->getMaterials();
                for (const auto& [key, material] : *materials)
                {
					std::string name = material->getName().empty() ? key : material->getName();
                    if (ImGui::Selectable(name.c_str()))
                    {
                        mesh->setMaterial(material);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::Text("No material assigned");
            if (ImGui::Button("Assign Material"))
            {
                ImGui::OpenPopup("Select Mesh Material");
            }

            // Material selection popup for mesh
            if (ImGui::BeginPopup("Select Mesh Material"))
            {
                auto materials = resourceManager->getMaterials();
                for (const auto& [key, material] : *materials)
                {
					std::string name = material->getName().empty() ? key : material->getName();
                    if (ImGui::Selectable(name.c_str()))
                    {
                        mesh->setMaterial(material);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
        }
        */
    }
}

void se::ImGuiManager::renderMaterialProperties()
{
    static char nameBuffer[128] = { 0 };

    auto materials = resourceManager->getMaterials();
    auto it = materials->find(selectedAssetName);
    if (it != materials->end())
    {
        auto material = it->second;

        strcpy_s(nameBuffer, sizeof(nameBuffer), material->getName().c_str());

        ImGui::Text("Material: ", material->getName().c_str());
		ImGui::SameLine();
		
        if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer))) {
            material->setName(nameBuffer);
        }

        ImGui::Text("GUID: %s", material->getGUID().c_str());
        ImGui::Text("Type: PBR");
        ImGui::Separator();

        renderMaterialEditor(material);
    }
}

void se::ImGuiManager::renderTextureProperties()
{
    static char nameBuffer[128] = { 0 };

    auto textures = resourceManager->getTextures();
    auto it = textures->find(selectedAssetName);
    if (it != textures->end())
    {
        auto texture = it->second;
        
        strcpy_s(nameBuffer, sizeof(nameBuffer), texture->getName().c_str());

        ImGui::Text("Texture: ", texture->getName().c_str());
        ImGui::SameLine();

        if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer))) {
            texture->setName(nameBuffer);
        }

        ImGui::Text("GUID: %s", texture->getGUID().c_str());
        ImGui::Separator();

        // Texture-specific properties would go here
        ImGui::Text("Format: RGBA");
        ImGui::Text("Mip Levels: N/A"); // Would need to expose this from SETexture
        ImGui::Text("Filter Mode: Linear");
        ImGui::Text("Wrap Mode: Repeat");
    }
}

void se::ImGuiManager::renderMaterialEditor(std::shared_ptr<se::SEMaterial> material)
{
    if (!material) return;

    // PBR Properties
    float metallic = material->getMetallic();
    float roughness = material->getRoughness();
    float ao = material->getAO();

    if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
    {
        material->setMetallic(metallic);
    }

    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
    {
        material->setRoughness(roughness);
    }

    if (ImGui::SliderFloat("Ambient Occlusion", &ao, 0.0f, 1.0f))
    {
        material->setAO(ao);
    }

    ImGui::Separator();
    ImGui::Text("Textures:");

    // Diffuse Texture
    renderTextureSelector("Diffuse", material->getDiffuseTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setDiffuseTexture(tex);
        });

    // Normal Texture
    renderTextureSelector("Normal", material->getNormalTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setNormalTexture(tex);
        });

    // Metallic Texture
    renderTextureSelector("Metallic", material->getMetallicTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setMetallicTexture(tex);
        });

    // Roughness Texture
    renderTextureSelector("Roughness", material->getRoughnessTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setRoughnessTexture(tex);
        });

    // AO Texture
    renderTextureSelector("AO", material->getAOTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setAOTexture(tex);
        });

    // Update material if needed
    material->update();
}

void se::ImGuiManager::renderTextureSelector(const std::string& label,
    std::shared_ptr<se::SETexture> currentTexture,
    std::function<void(std::shared_ptr<se::SETexture>)> onTextureSelected)
{
    ImGui::PushID(label.c_str());

    if (currentTexture)
    {
        ImGui::Text("%s: %s", label.c_str(), currentTexture->getName().c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Change"))
        {
            ImGui::OpenPopup("Select Texture");
        }
    }
    else
    {
        ImGui::Text("%s: None", label.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Assign"))
        {
            ImGui::OpenPopup("Select Texture");
        }
    }

    // Texture selection popup
    if (ImGui::BeginPopup("Select Texture"))
    {
        auto textures = resourceManager->getTextures();
        for (const auto& [key, texture] : *textures)
        {
			std::string name = texture->getName().empty() ? key : texture->getName();
            if (ImGui::Selectable(name.c_str()))
            {
                onTextureSelected(texture);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

void se::ImGuiManager::init(SEDevice& seDevice, VkRenderPass renderPass, GLFWwindow* window, std::vector<se::SEGameObject>* gameobjects, se::ResourceManager* resourceManager)
{
    this->window = window;
    this->renderPass = renderPass;
    this->gameobjects = gameobjects;
    this->resourceManager = resourceManager;
    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = seDevice.getInstance();
    initInfo.PhysicalDevice = seDevice.physicaldevice();
    initInfo.Device = seDevice.device();
    initInfo.QueueFamily = seDevice.findPhysicalQueueFamilies().graphicsFamily.value();
    initInfo.Queue = seDevice.graphicsQueue();;
    initInfo.RenderPass = renderPass;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = seDevice.getDescriptorPool();
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 2;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&initInfo);
    // Load Fonts
    io.Fonts->AddFontDefault();
}

void se::ImGuiManager::newFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void se::ImGuiManager::render(VkCommandBuffer commandBuffer)
{
    renderSceneHierarchy();
    renderAssetBrowser();
    renderAssetViewer();
    renderPropertiesPanel();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void se::ImGuiManager::cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}