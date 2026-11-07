#pragma once  
#include <vulkan/vulkan.h>

#include <string>
#include <unordered_map>  
#include <memory>  

namespace se
{
	class SEDevice;
	class TextureSystem;
	class SEMaterial;

	class MaterialSystem
	{
	public:
		MaterialSystem(se::SEDevice& device, se::TextureSystem& textures,
			VkPipelineLayout pbrPipelineLayout, VkPipeline pbrPipeline,
			VkDescriptorSetLayout pbrDescriptorSetLayout);
		~MaterialSystem();

		MaterialSystem(const MaterialSystem&) = delete;
		MaterialSystem& operator=(const MaterialSystem&) = delete;
		MaterialSystem(MaterialSystem&&) = delete;
		MaterialSystem& operator=(MaterialSystem&&) = delete;

		std::unordered_map<std::string, std::shared_ptr<se::SEMaterial>>* getMaterials()
		{
			return &materials;
		}

		std::shared_ptr<se::SEMaterial> CreatePBRMaterial(const std::string guid, const std::string name);
		//std::shared_ptr<se::SEMaterial> CreateMaterial(const std::string& name, const std::string& vertexShader, const std::string& fragmentShader);  


	private:
		se::SEDevice& seDevice;
		se::TextureSystem& textureSystem;

		std::unordered_map<std::string, std::shared_ptr<se::SEMaterial>> materials;

		VkPipelineLayout pbrPipelineLayout{VK_NULL_HANDLE};
		VkPipeline pbrPipeline{VK_NULL_HANDLE};
		VkDescriptorSetLayout pbrDescriptorSetLayout{VK_NULL_HANDLE};
		//Add more descriptor sets for more material types  
	};
} // namespace se
