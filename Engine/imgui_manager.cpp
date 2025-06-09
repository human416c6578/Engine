#include "imgui_manager.hpp"
#include "se_pbr.hpp"

void se::ImGuiManager::renderSceneHierarchy()
{
	static int cubeCount = 0;
    static int sphereCount = 0;
    static int lightCount = 0;
    bool itemHovered = false;
    auto scene = sceneManager->getActiveScene();
    std::vector<std::unique_ptr<SEGameObject>>& gameobjects = scene->getGameObjects();

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

    ImGui::Text("Game Objects (%d)", gameobjects.size());
    ImGui::Separator();

    for (int i = 0; i < gameobjects.size(); i++)
    {
        auto& gameObject = gameobjects.at(i);
        ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
        bool isSelected = (selectedGameObjectIndex == i);

        if (ImGui::Selectable((gameObject->getName() + "##" +std::to_string(gameObject->getId())).c_str(), isSelected, flags))
        {
            selectedGameObjectIndex = i;
            selectedAssetName = "";
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            itemHovered = true;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                gameobjects.erase(gameobjects.begin() + i);
                if (selectedGameObjectIndex == i) selectedGameObjectIndex = -1;
                else if (selectedGameObjectIndex > i) selectedGameObjectIndex--;
            }
            ImGui::EndPopup();
        }
    }

    if (!itemHovered && ImGui::BeginPopupContextWindow()) {
        if (ImGui::MenuItem("Create Empty")) {
            scene->createGameObject("Empty");
            
        }
        if (ImGui::MenuItem("Create Cube")) {
            auto& go = scene->createGameObject("Cube");
            go.setMesh(resourceManager->createCube("CubeMesh_" + std::to_string(++cubeCount)));
            go.setMaterial(resourceManager->createMaterial("CubeMaterial_" + std::to_string(cubeCount)));
        }
        if (ImGui::MenuItem("Create Sphere")) {
            auto& go = scene->createGameObject("Sphere");
            go.setMesh(resourceManager->createSphere("SphereMesh_" + std::to_string(++sphereCount)));
            go.setMaterial(resourceManager->createMaterial("SphereMaterial_" + std::to_string(sphereCount)));

        }
        if (ImGui::MenuItem("Create Light")) {
            auto& go = scene->createGameObject("Light");
            Light light{};
			light.type = LightType::Point;
			light.color = { 1.0f, 1.0f, 1.0f };
			light.intensity = 1.0f;
            go.setLight(light);
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
           /* selectedAssetName = "";
            selectedGameObjectIndex = -1;*/
        }
    }

    renderAssetContextMenu();

    ImGui::End();
}

void se::ImGuiManager::renderAssetContextMenu()
{
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
            config.countSelectionMax = 0;
            config.flags = ImGuiFileDialogFlags_CaseInsensitiveExtentionFiltering;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModel", "Select Model Files", "Model Files{.obj,.fbx}", config);

        }
        if (ImGui::MenuItem("Load Texture")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 0;
            config.flags = ImGuiFileDialogFlags_CaseInsensitiveExtentionFiltering;

            ImGuiFileDialog::Instance()->OpenDialog("ChooseTexture", "Select Texture Files", "Texture Files{.png,.jpg,.jpeg,.tga,.bmp,.hdr}", config);

        }
        if (ImGui::MenuItem("Load Scene"))
        {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 0;
            config.flags = ImGuiFileDialogFlags_CaseInsensitiveExtentionFiltering;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseScene", "Select Scene File", "Scene File{.json}", config);

        }
        if (ImGui::MenuItem("Save Scene")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
            ImGuiFileDialog::Instance()->OpenDialog("SaveScene", "Save Scene File", "Scene File{.json}", config);
        }
        ImGui::EndPopup();
    }

    

    if (ImGuiFileDialog::Instance()->Display("ChooseModel", ImGuiWindowFlags_NoCollapse, ImVec2(400.0f, 200.0f))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

            resourceManager->loadMesh(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseTexture", ImGuiWindowFlags_NoCollapse, ImVec2(400.0f, 200.0f))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            auto selection = ImGuiFileDialog::Instance()->GetSelection();

            for (const auto& [filename, filepathname] : selection)
            {

                resourceManager->loadTexture(filepathname);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseScene", ImGuiWindowFlags_NoCollapse, ImVec2(400.0f, 200.0f))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            auto selection = ImGuiFileDialog::Instance()->GetSelection();
            if (!selection.empty()) {
                const auto& [filename, filepathname] = *selection.begin();
                sceneManager->loadScene(filepathname, filename);
                sceneManager->setActiveScene(filename);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("SaveScene", ImGuiWindowFlags_NoCollapse, ImVec2(400.0f, 200.0f))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            sceneManager->saveActiveScene(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void se::ImGuiManager::renderAssetViewer()
{
    if (!showAssetViewer) return;

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    float left = workPos.x + 300.0f;
    float right = workPos.x + workSize.x - 300.0f;
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
        static char searchBuf[128] = "";
        ImGui::InputTextWithHint("##Search", "Search textures...", searchBuf, IM_ARRAYSIZE(searchBuf));

        ImGui::Separator();

        const float thumbnailSize = 64.0f;
        const float padding = 8.0f;
        float cellSize = thumbnailSize + padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        ImGui::Columns(columnCount, nullptr, false);

        auto textures = resourceManager->getTextures();
        for (const auto& [key, texture] : *textures)
        {
            const std::string& name = texture->getName().empty() ? key : texture->getName();
            if (strlen(searchBuf) > 0 && name.find(searchBuf) == std::string::npos)
                continue;

            ImGui::PushID(texture.get());

            ImVec2 cursor = ImGui::GetCursorScreenPos();

            // Draw the texture thumbnail
            ImGui::Image((ImTextureID)texture->getTextureDescriptorSet(), { thumbnailSize, thumbnailSize });

            // Draw hover highlight
            if (ImGui::IsItemHovered())
            {
                ImGui::GetWindowDrawList()->AddRect(
                    cursor,
                    ImVec2(cursor.x + thumbnailSize, cursor.y + thumbnailSize),
                    IM_COL32(0, 155, 255, 255),
                    4.0f,
                    0,
                    2.0f
                );

                ImGui::BeginTooltip();
                ImGui::Text("Texture: %s", name.c_str());
                ImGui::Text("GUID: %s", texture->getGUID().c_str());
                ImGui::EndTooltip();
            }

            // Handle selection
            if (ImGui::IsItemClicked())
            {
                selectedAssetName = key;
                selectedGameObjectIndex = -1;
                ImGui::CloseCurrentPopup(); // Optional: only if in a popup
            }

            // Center the name below the image
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            float textOffset = std::max(0.0f, (thumbnailSize - textSize.x) * 0.5f);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);
            ImGui::TextWrapped("%s", name.c_str());

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
    }
    else if (selectedAssetCategory == "Scripts")
    {
        for (auto& [name, factory] : se::getScriptRegistry()) {
            if (ImGui::Selectable(name.c_str())) {
                auto script = factory();

                ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
                bool isSelected = (selectedAssetName == name);

                if (ImGui::Selectable(name.c_str(), isSelected, flags))
                {
                    selectedAssetName = name;
                    selectedGameObjectIndex = -1;
                }
            }
        }
    }

    renderAssetContextMenu();

    ImGui::End();
}

void se::ImGuiManager::renderPropertiesPanel()
{
    if (!showProperties) return;

    auto scene = sceneManager->getActiveScene();
    std::vector<std::unique_ptr<SEGameObject>>& gameobjects = scene->getGameObjects();

    auto* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;

    // Fixed to right, full height, 400px wide
    ImGui::SetNextWindowPos(ImVec2(workPos.x + workSize.x - 300, workPos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, workSize.y), ImGuiCond_Always);

    ImGui::Begin("Properties", &showProperties,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (selectedGameObjectIndex >= 0 && selectedGameObjectIndex < gameobjects.size())
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
    auto scene = sceneManager->getActiveScene();
    std::vector<std::unique_ptr<SEGameObject>>& gameobjects = scene->getGameObjects();

    auto& gameObject = gameobjects[selectedGameObjectIndex];
    static char nameBuffer[128] = { 0 };

    strcpy_s(nameBuffer, sizeof(nameBuffer), gameObject->getName().c_str());

    ImGui::Text("GameObject: ", gameObject->getName().c_str());
    ImGui::SameLine();

    if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer))) {
        gameObject->setName(nameBuffer);
    }

    std::string name = gameObject->getName().empty() ?
        ("GameObject_" + std::to_string(selectedGameObjectIndex)) : gameObject->getName();

    ImGui::Text("GameObject: %s", name.c_str());
    ImGui::Separator();

    renderTransformComponent(gameObject);
    renderMeshComponent(gameObject);
	renderMaterialComponent(gameObject);
    renderLightComponent(gameObject);
    renderScriptComponent(gameObject);
}

void se::ImGuiManager::renderTransformComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto transform = gameObject->getTransform();

        float position[3] = { transform.translation.x, transform.translation.y, transform.translation.z };
        float rotation[3] = { transform.rotation.x, transform.rotation.y, transform.rotation.z };
        float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };

        bool changed = false;

        if (ImGui::DragFloat3("Position", position, 0.1f)) {
            changed = true;
        }
        if (ImGui::DragFloat3("Rotation", rotation, 0.1f)) {
            changed = true;
        }
        if (ImGui::DragFloat3("Scale", scale, 0.01f)) {
            changed = true;
        }

        if (changed) {
            TransformComponent newTransform = transform;
            newTransform.translation = { position[0], position[1], position[2] };
            newTransform.rotation = { rotation[0], rotation[1], rotation[2] };
            newTransform.scale = { scale[0], scale[1], scale[2] };
            gameObject->setTransform(newTransform);
        }

    }
}

void se::ImGuiManager::renderMeshComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (ImGui::CollapsingHeader("Mesh"))
    {
        auto mesh = gameObject->getMesh();

        renderMeshSelector(mesh,
            [&gameObject](std::shared_ptr<se::SEMesh> mesh) {
                gameObject->setMesh(mesh);
            });

        if (mesh)
        {
            ImGui::Text("GUID: %s", mesh->getGUID().c_str());
            ImGui::Separator();

            // Mesh-specific properties
            ImGui::Text("Submeshes: %d", mesh->getSubMeshCount());
            ImGui::Text("Vertices: %d", mesh->getVerticesCount());
            ImGui::Text("Triangles: %d", mesh->getIndicesCount());
        }
    }
}

void se::ImGuiManager::renderMaterialComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (ImGui::CollapsingHeader("Material"))
    {
        auto material = gameObject->getMaterial();

        renderMaterialSelector(material,
            [&gameObject](std::shared_ptr<se::SEMaterial> mat) {
                gameObject->setMaterial(mat);
            });

        if (material)
        {
            ImGui::Separator();
            renderMaterialEditor(material);
        }

    }
}

void se::ImGuiManager::renderLightComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    if (!gameObject->hasLight()) return;

    if (ImGui::CollapsingHeader("Light"))
    {
        auto& light = gameObject->getLight();

        // Light type selector
        static const char* lightTypeNames[] = {"Point", "Directional", "Spot" };
        int typeIndex = static_cast<int>(light.type) - 1;
        if (ImGui::Combo("Type", &typeIndex, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
        {
            light.type = static_cast<se::LightType>(typeIndex+1);
        }

        // Color picker
        if (ImGui::ColorEdit3("Color##", &light.color.x, ImGuiColorEditFlags_Float))
        {
            // Clamp to [0, 1] in case user enters out-of-range values
            light.color.r = std::clamp(light.color.r, 0.0f, 1.0f);
            light.color.g = std::clamp(light.color.g, 0.0f, 1.0f);
            light.color.b = std::clamp(light.color.b, 0.0f, 1.0f);
        }

        // Intensity slider
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f, "%.2f");

        // Direction (for Directional and Spot lights)
        if (light.type == se::LightType::Directional || light.type == se::LightType::Spot)
        {
            ImGui::DragFloat3("Direction", &light.direction.x, 0.1f, -1.0f, 1.0f);
        }

        // Spot angle (for Spot lights)
        if (light.type == se::LightType::Spot)
        {
            ImGui::SliderFloat("Spot Angle", &light.spotAngle, 0.0f, 90.0f, "%.1f deg");
        }
    }
}

void se::ImGuiManager::renderScriptComponent(std::unique_ptr<se::SEGameObject>& gameObject)
{
    //if (!gameObject->hasScript()) return;

    if (ImGui::CollapsingHeader("Script"))
    {
        auto& script = gameObject->getScript();

        renderScriptSelector(script,
            [&gameObject](std::unique_ptr<se::ScriptComponent> selectedScript) {
                gameObject->setScript(std::move(selectedScript));
            });
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

        ImGui::Image((ImTextureID)texture->getTextureDescriptorSet(), ImVec2(128, 128));
    }
}

void se::ImGuiManager::renderMaterialEditor(std::shared_ptr<se::SEMaterial> material)
{
    if (!material) return;

    // PBR Properties
	glm::vec3 color = material->getColor();
    float metallic = material->getMetallic();
    float roughness = material->getRoughness();
    float ao = material->getAO();

    // Color picker
    if (ImGui::ColorEdit3("Color", &color.x, ImGuiColorEditFlags_Float))
    {
        // Clamp to [0, 1] in case user enters out-of-range values
        color.r = std::clamp(color.r, 0.0f, 1.0f);
        color.g = std::clamp(color.g, 0.0f, 1.0f);
        color.b = std::clamp(color.b, 0.0f, 1.0f);

		material->setColor(color);
    }

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
    ImGui::Separator();
    // Normal Texture
    renderTextureSelector("Normal", material->getNormalTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setNormalTexture(tex);
        });
    ImGui::Separator();
    // Metallic Texture
    renderTextureSelector("Metallic", material->getMetallicTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setMetallicTexture(tex);
        });
    ImGui::Separator();
    // Roughness Texture
    renderTextureSelector("Roughness", material->getRoughnessTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setRoughnessTexture(tex);
        });
    ImGui::Separator();
    // AO Texture
    renderTextureSelector("AO", material->getAOTexture(),
        [material](std::shared_ptr<se::SETexture> tex) {
            material->setAOTexture(tex);
        });
}

void se::ImGuiManager::renderTextureSelector(const std::string& label,
    std::shared_ptr<se::SETexture> currentTexture,
    std::function<void(std::shared_ptr<se::SETexture>)> onTextureSelected)
{
    ImGui::PushID(label.c_str());

    const float thumbnailSize = 24;
    const ImVec2 imageSize(thumbnailSize, thumbnailSize);
    bool openPopup = false;

    ImGui::BeginGroup();

    // Thumbnail image
    if (currentTexture) {
        ImTextureID descriptor = (ImTextureID)currentTexture->getTextureDescriptorSet();
        if (ImGui::ImageButton(("##" + label).c_str(), descriptor, imageSize)) {
            openPopup = true;
        }
    }
    else {
        // Empty/placeholder preview
        ImVec4 borderColor = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        if (ImGui::InvisibleButton("NoTexture", imageSize)) {
            openPopup = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("No texture assigned");
        }
        ImGui::GetWindowDrawList()->AddRect(
            ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(borderColor));
    }

    ImGui::SameLine();

    // Label (e.g., "Diffuse", "Normal") as clickable
    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(100, 0))) {
        openPopup = true;
    }

    ImGui::EndGroup();

    if (openPopup) {
        ImGui::OpenPopup("Select Texture");
    }

    // Texture selection popup
    if (ImGui::BeginPopup("Select Texture"))
    {
        static char searchBuf[128] = "";
        ImGui::InputTextWithHint("##Search", "Search textures...", searchBuf, IM_ARRAYSIZE(searchBuf));

        ImGui::Separator();
        const float thumbnailSize = 64.0f;
        const float padding = 8.0f;
        float cellSize = thumbnailSize + padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        ImGui::BeginChild("TextureGrid", ImVec2(0, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(columnCount, nullptr, false);

        auto textures = resourceManager->getTextures();
        for (const auto& [key, texture] : *textures)
        {
            const std::string& name = texture->getName().empty() ? key : texture->getName();
            if (strlen(searchBuf) > 0 && name.find(searchBuf) == std::string::npos)
                continue;

            ImGui::PushID(texture.get());

            ImVec2 imagePos = ImGui::GetCursorScreenPos();
            ImGui::Image((ImTextureID)texture->getTextureDescriptorSet(), { thumbnailSize, thumbnailSize });

            if (ImGui::IsItemHovered())
            {
                ImGui::GetWindowDrawList()->AddRect(
                    imagePos,
                    ImVec2(imagePos.x + thumbnailSize, imagePos.y + thumbnailSize),
                    IM_COL32(0, 155, 255, 255), // Blue border
                    4.0f,                       // Corner rounding
                    0,
                    2.0f                        // Thickness
                );

                ImGui::BeginTooltip();
                ImGui::Text("Texture: %s", name.c_str());
                ImGui::Text("GUID: %s", texture->getGUID().c_str());
                ImGui::EndTooltip();
            }

            if (ImGui::IsItemClicked())
            {
                onTextureSelected(texture);
                ImGui::CloseCurrentPopup();
            }

            ImGui::TextWrapped("%s", name.c_str());

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::EndPopup();
    }


    ImGui::PopID();
}

void se::ImGuiManager::renderMaterialSelector(
    std::shared_ptr<se::SEMaterial> currentMaterial,
    std::function<void(std::shared_ptr<se::SEMaterial>)> onMaterialSelected)
{
	std::string label = currentMaterial ? currentMaterial->getName() : "No Material";
    ImGui::PushID(label.c_str());

    bool openPopup = false;

    ImGui::BeginGroup();

    // Label (e.g., "Diffuse", "Normal") as clickable
    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(100, 0))) {
        openPopup = true;
    }

    ImGui::EndGroup();

    if (openPopup) {
        ImGui::OpenPopup("Select Material");
    }

    // Texture selection popup
    if (ImGui::BeginPopup("Select Material"))
    {
        static char searchBuf[128] = "";
        ImGui::InputTextWithHint("##Search", "Search materials...", searchBuf, IM_ARRAYSIZE(searchBuf));

        ImGui::Separator();

        ImGui::BeginChild("TextureGrid", ImVec2(0, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(1, nullptr, false);

        if (ImGui::Selectable("None", false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, 0)))
        {
            onMaterialSelected(nullptr);
            ImGui::CloseCurrentPopup();
        }

        auto materials = resourceManager->getMaterials();
        for (const auto& [key, material] : *materials)
        {
            const std::string& name = material->getName().empty() ? key : material->getName();
            if (strlen(searchBuf) > 0 && name.find(searchBuf) == std::string::npos)
                continue;

            ImGui::PushID(material.get());

            if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, 0)))
            {
                onMaterialSelected(material);
                ImGui::CloseCurrentPopup();
            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::EndPopup();
    }


    ImGui::PopID();
}

void se::ImGuiManager::renderMeshSelector(
    std::shared_ptr<se::SEMesh> currentMesh,
    std::function<void(std::shared_ptr<se::SEMesh>)> onMeshSelected)
{
    std::string label = currentMesh ? currentMesh->getName() : "No Mesh";
    ImGui::PushID(label.c_str());

    bool openPopup = false;

    ImGui::BeginGroup();

    // Label (e.g., "Diffuse", "Normal") as clickable
    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(100, 0))) {
        openPopup = true;
    }

    ImGui::EndGroup();

    if (openPopup) {
        ImGui::OpenPopup("Select Mesh");
    }

    // Texture selection popup
    if (ImGui::BeginPopup("Select Mesh"))
    {
        static char searchBuf[128] = "";
        ImGui::InputTextWithHint("##Search", "Search meshes...", searchBuf, IM_ARRAYSIZE(searchBuf));

        ImGui::Separator();

        ImGui::BeginChild("TextureGrid", ImVec2(0, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(1, nullptr, false);

        auto meshes = resourceManager->getMeshes();
        for (const auto& [key, mesh] : *meshes)
        {
            const std::string& name = mesh->getName().empty() ? key : mesh->getName();
            if (strlen(searchBuf) > 0 && name.find(searchBuf) == std::string::npos)
                continue;

            ImGui::PushID(mesh.get());

            if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, 0)))
            {

                onMeshSelected(mesh);
                ImGui::CloseCurrentPopup();

            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::EndPopup();
    }


    ImGui::PopID();
}

void se::ImGuiManager::renderScriptSelector(
    std::unique_ptr<se::ScriptComponent>& currentScript,
    std::function<void(std::unique_ptr<se::ScriptComponent>)> onScriptSelected)
{
    std::string label = currentScript ? currentScript->getName() : "No Script";
    ImGui::PushID(label.c_str());

    bool openPopup = false;

    ImGui::BeginGroup();

    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(100, 0))) {
        openPopup = true;
    }

    ImGui::EndGroup();

    if (openPopup) {
        ImGui::OpenPopup("Select Script");
    }

    if (ImGui::BeginPopup("Select Script"))
    {
        static char searchBuf[128] = "";
        ImGui::InputTextWithHint("##Search", "Search scripts...", searchBuf, IM_ARRAYSIZE(searchBuf));
        ImGui::Separator();

        ImGui::BeginChild("ScriptList", ImVec2(0, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(1, nullptr, false);

        // Option to remove script
        if (ImGui::Selectable("None", false)) {
            onScriptSelected(nullptr);
            ImGui::CloseCurrentPopup();
        }

        // Iterate over all registered scripts
        for (const auto& [name, factory] : se::getScriptRegistry())
        {
            if (strlen(searchBuf) > 0 && name.find(searchBuf) == std::string::npos)
                continue;

            ImGui::PushID(name.c_str());

            if (ImGui::Selectable(name.c_str(), false)) {
                onScriptSelected(factory());
                ImGui::CloseCurrentPopup();
            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::EndPopup();
    }

    ImGui::PopID();
}


void se::ImGuiManager::init(SEDevice& seDevice, VkRenderPass renderPass, GLFWwindow* window, se::ResourceManager* resourceManager) 
{
    sceneManager = &se::SceneManager::getInstance();

    this->window = window;
    this->renderPass = renderPass;
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