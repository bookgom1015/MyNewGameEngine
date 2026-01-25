#ifndef __SAMPLERUTIL_INL__
#define __SAMPLERUTIL_INL__

namespace Render::DX11::Shading::Util {
	constexpr ID3D11SamplerState** SamplerUtil::GetSamplers() noexcept { return msSamplerPtrs; }

	constexpr UINT SamplerUtil::SamplerCount() noexcept { return SamplerState::Type::Count; }
}

#endif // __SAMPLERUTIL_INL__