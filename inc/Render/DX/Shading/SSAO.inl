#ifndef __SSAO_INL__
#define __SSAO_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::SSAO::SSAOClass::AOMap(UINT index) const {
	return mAOMaps[index].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::SSAO::SSAOClass::AOMapSrv(UINT index) const {
	return mhAOMapGpuSrvs[index];
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::SSAO::SSAOClass::AOMapUav(UINT index) const {
	return mhAOMapGpuUavs[index];
}

void Render::DX::Shading::SSAO::SSAOClass::GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]) {
	std::copy(&mOffsets[0], &mOffsets[14], &offsets[0]);
}

constexpr UINT Render::DX::Shading::SSAO::SSAOClass::TexWidth() const {
	return mTexWidth;
}

constexpr UINT Render::DX::Shading::SSAO::SSAOClass::TexHeight() const {
	return mTexHeight;
}

#endif // __SSAO_INL__