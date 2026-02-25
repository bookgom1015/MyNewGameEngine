#ifndef __SHADOW_INL__
#define __SHADOW_INL__

namespace Render::DX::Shading::Shadow {
	Common::Foundation::Light* ShadowClass::Light(UINT index) const {
		return mLights[index].get();
	}

	constexpr UINT ShadowClass::LightCount() const {
		return mLightCount;
	}

	Render::DX::Foundation::Resource::GpuResource* ShadowClass::ShadowMap() const {
		return mShadowMap.get();
	}

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE ShadowClass::ShadowMapSrv() const {
		return mhShadowMapGpuSrv;
	}

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE ShadowClass::ShadowMapUav() const {
		return mhShadowMapGpuUav;
	}

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE ShadowClass::ZDepthMapSrv() const {
		return mhZDepthMapGpuSrvs[0];
	}
}

#endif // __SHADOW_INL__