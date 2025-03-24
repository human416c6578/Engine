#include "se_cubemap.hpp"

namespace se
{
	SECubemap::SECubemap(SEDevice& device, SERenderer& renderer, const std::string& path) : seDevice{ device }, seRenderer{ renderer }, seCubemapConverter{device, renderer, path}
	{
		
	}
}

