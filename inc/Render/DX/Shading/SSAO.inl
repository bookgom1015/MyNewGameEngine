#ifndef __SSAO_INL__
#define __SSAO_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::SSAO::SSAOClass::AOCoefficientResource(Resource::AO::Type type) const {
	return mAOResources[type].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::SSAO::SSAOClass::AOCoefficientDescriptor(Descriptor::AO::Type type) const {
	return mhAOResourceGpus[type];
}

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::SSAO::SSAOClass::TemporalAOCoefficientResource(UINT frame) const {
	return mTemporalAOResources[frame].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::SSAO::SSAOClass::TemporalAOCoefficientSrv(UINT frame) const {
	return mhTemporalAOResourceGpus[frame][Descriptor::TemporalAO::E_Srv];
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::SSAO::SSAOClass::TemporalAOCoefficientUav(UINT frame) const {
	return mhTemporalAOResourceGpus[frame][Descriptor::TemporalAO::E_Uav];
}

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::SSAO::SSAOClass::TemporalCacheResource(Resource::TemporalCache::Type type, UINT frame) const {
	return mTemporalCaches[frame][type].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::SSAO::SSAOClass::TemporalCacheDescriptor(Descriptor::TemporalCache::Type type, UINT frame) const {
	return mhTemporalCacheGpus[frame][type];
}

constexpr UINT Render::DX::Shading::SSAO::SSAOClass::CurrentTemporalCacheFrameIndex() const {
	return mCurrentTemporalCacheFrameIndex;
}

constexpr UINT Render::DX::Shading::SSAO::SSAOClass::CurrentTemporalAOFrameIndex() const {
	return mCurrentTemporalAOFrameIndex;
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