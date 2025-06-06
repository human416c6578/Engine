#include "se_material_system.hpp"

namespace se
{
	MaterialSystem::MaterialSystem(se::SEDevice& device, VkPipelineLayout pbrPipelineLayout, VkPipeline pbrPipeline, VkDescriptorSetLayout pbrDescriptorSetLayout) : seDevice{ device }, pbrPipelineLayout{pbrPipelineLayout}, pbrPipeline{pbrPipeline}, pbrDescriptorSetLayout{pbrDescriptorSetLayout}
	{
	
	}

	MaterialSystem::~MaterialSystem()
	{
	}

	std::shared_ptr<se::SEMaterial> MaterialSystem::CreatePBRMaterial(const std::string guid, const std::string name)
	{
		auto material = std::make_shared<se::SEMaterial>(
			seDevice, pbrPipelineLayout, pbrPipeline, pbrDescriptorSetLayout, VK_SAMPLE_COUNT_1_BIT, guid, name, 0.0, 0.95, 1.0, textureSystem->getDummyTexture());
		materials.insert({ guid, material });

		return material;
	}
}


