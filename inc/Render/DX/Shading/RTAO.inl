#ifndef __RTAO_INL__
#define __RTAO_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RTAO::RTAOClass::AOCoefficientResource(Resource::AO::Type type) const {
	return mAOResources[type].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RTAO::RTAOClass::AOCoefficientDescriptor(Descriptor::AO::Type type) const {
	return mhAOResourceGpus[type];
}

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RTAO::RTAOClass::TemporalAOCoefficientResource(UINT frame) const {
	return mTemporalAOResources[frame].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RTAO::RTAOClass::TemporalAOCoefficientSrv(UINT frame) const {
	return mhTemporalAOResourceGpus[frame][Descriptor::TemporalAO::E_Srv];
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RTAO::RTAOClass::TemporalAOCoefficientUav(UINT frame) const {
	return mhTemporalAOResourceGpus[frame][Descriptor::TemporalAO::E_Uav];
}

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RTAO::RTAOClass::TemporalCacheResource(Resource::TemporalCache::Type type, UINT frame) const {
	return mTemporalCaches[frame][type].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RTAO::RTAOClass::TemporalCacheDescriptor(Descriptor::TemporalCache::Type type, UINT frame) const {
	return mhTemporalCacheGpus[frame][type];
}

constexpr UINT Render::DX::Shading::RTAO::RTAOClass::CurrentTemporalCacheFrameIndex() const {
	return mCurrentTemporalCacheFrameIndex;
}

constexpr UINT Render::DX::Shading::RTAO::RTAOClass::CurrentTemporalAOFrameIndex() const {
	return mCurrentTemporalAOFrameIndex;
}

#endif // __RTAO_INL__