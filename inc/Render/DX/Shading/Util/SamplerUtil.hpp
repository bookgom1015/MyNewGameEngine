#pragma once

namespace Render::DX::Shading::Util {
	static const UINT StaticSamplerCount = 11;	

	using StaticSamplers = std::array<const D3D12_STATIC_SAMPLER_DESC, StaticSamplerCount>;

	class SamplerUtil {
	public:
		static const D3D12_STATIC_SAMPLER_DESC* GetStaticSamplers() noexcept;

	private:
		static StaticSamplers msStaticSamplers;
	};
}