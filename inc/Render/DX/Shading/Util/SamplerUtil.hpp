#pragma once

#include <array>

#include <d3d12.h>

namespace Render::DX::Shading::Util {
	using StaticSamplers = std::array<const D3D12_STATIC_SAMPLER_DESC, 11>;

	class SamplerUtil {
	public:
		static StaticSamplers GetStaticSamplers();
	};
}