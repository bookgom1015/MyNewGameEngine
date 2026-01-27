#ifndef __ENVIRONMENTMAP_INL__
#define __ENVIRONMENTMAP_INL__

namespace Render::DX11::Shading::EnvironmentMap {
	ID3D11ShaderResourceView* EnvironmentMapClass::DiffuseIrradianceMapSrv() {
		return mhDiffuseIrradianceCubeMapSrv.Get();
	}

	ID3D11ShaderResourceView* EnvironmentMapClass::PrefilteredEnvironmentCubeMapSrv() {
		return mhPrefilteredEnvironmentCubeMapSrv.Get();
	}

	ID3D11ShaderResourceView* EnvironmentMapClass::BrdfLutMapSrv() {
		return mhBrdfLutMapSrv.Get();
	}
}

#endif // __ENVIRONMENTMAP_INL__