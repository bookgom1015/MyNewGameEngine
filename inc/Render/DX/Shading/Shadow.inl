#ifndef __SHADOW_INL__
#define __SHADOW_INL__

constexpr const Render::DX::Foundation::Light* Render::DX::Shading::Shadow::ShadowClass::Lights() const {
	return mLights.data();
}

constexpr Render::DX::Foundation::Light Render::DX::Shading::Shadow::ShadowClass::Light(UINT index) const {
	return mLights[index];
}

constexpr UINT Render::DX::Shading::Shadow::ShadowClass::LightCount() const {
	return mLightCount;
}

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::Shadow::ShadowClass::ShadowMap() const {
	return mShadowMap.get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::Shadow::ShadowClass::ShadowMapSrv() const {
	return mhShadowMapGpuSrv;
}

#endif // __SHADOW_INL__