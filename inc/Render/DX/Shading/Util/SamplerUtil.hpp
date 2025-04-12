#pragma once

#include <array>

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3d12.h>

namespace Render::DX::Shading::Util {
	using StaticSamplers = std::array<const D3D12_STATIC_SAMPLER_DESC, 11>;

	class SamplerUtil {
	public:
		static StaticSamplers GetStaticSamplers();
	};
}