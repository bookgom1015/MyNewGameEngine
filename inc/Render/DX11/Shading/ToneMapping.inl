#ifndef __TONEMAPPING_INL__
#define __TONEMAPPING_INL__

namespace Render::DX11::Shading::ToneMapping {
	ID3D11Texture2D* ToneMappingClass::InterMediateMap() {
		return mIntermediateMap.Get();
	}

	ID3D11ShaderResourceView* ToneMappingClass::InterMediateMapSrv() {
		return mhIntermediateMapSrv.Get();
	}

	ID3D11RenderTargetView* ToneMappingClass::InterMediateMapRtv() {
		return mhIntermediateMapRtv.Get();
	}
}

#endif // __TONEMAPPING_INL__