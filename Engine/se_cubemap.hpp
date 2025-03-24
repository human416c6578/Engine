#pragma once

#include "se_device.hpp"
#include "se_renderer.hpp"
#include "se_hdr_to_cubemap.h"
#include "se_cubemap_diffuse.h"
#include "se_cubemap_specular.h"


// std
#include <memory>
#include <vector>

namespace se
{
	class SECubemap
	{
	public:

		SECubemap(SEDevice& device, SERenderer& renderPass, const std::string& textureFilepath);

		~SECubemap()
		{

		}

		SECubemap(const SECubemap&) = delete;
		SECubemap& operator=(const SECubemap&) = delete;

	private:

		SEDevice& seDevice;
		SERenderer& seRenderer;
		SEHdrToCubemap seCubemapConverter;
		//SECubemapSpecular seSpecular;
		//SECubemapDiffuse seDiffuse;
	};

}
