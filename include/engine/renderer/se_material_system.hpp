#pragma once  
#include <string>  
#include <vector>  
#include <unordered_map>  
#include <memory>  

#include "se_pbr_material.hpp" 
#include "se_texture_system.hpp"

namespace se
{
	class MaterialSystem
	{
	public:
		MaterialSystem(se::SEDevice& device, VkPipelineLayout pbrPipelineLayout, VkPipeline pbrPipeline, VkDescriptorSetLayout pbrDescriptorSetLayout);
		~MaterialSystem();

		std::unordered_map<std::string, std::shared_ptr<se::SEMaterial>>* getMaterials()
		{
			return &materials;
		}

		void setTextureSystem(se::TextureSystem* textureSystem)
		{
			this->textureSystem = textureSystem;
		}

		std::shared_ptr<se::SEMaterial> CreatePBRMaterial(const std::string guid, const std::string name);
		//std::shared_ptr<se::SEMaterial> CreateMaterial(const std::string& name, const std::string& vertexShader, const std::string& fragmentShader);  


	private:
		se::SEDevice& seDevice;
		se::TextureSystem* textureSystem = nullptr;

		std::unordered_map<std::string, std::shared_ptr<se::SEMaterial>> materials;

		VkPipelineLayout pbrPipelineLayout;
		VkPipeline pbrPipeline;
		VkDescriptorSetLayout pbrDescriptorSetLayout;
		//Add more descriptor sets for more material types  
	};
} // namespace se