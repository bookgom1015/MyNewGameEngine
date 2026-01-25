#ifndef __SHADOW_INL__
#define __SHADOW_INL__

namespace Render::DX11::Shading {
	ID3D11ShaderResourceView* Shadow::ShadowClass::ShadowMapSrv() { return mhShadowMapSrv.Get(); }

	ID3D11UnorderedAccessView* Shadow::ShadowClass::ShadowMapUav() { return mhShadowMapUav.Get(); }

	constexpr UINT Shadow::ShadowClass::LightCount() const noexcept { return mLightCount; }

}

#endif // __SHADOW_INL__