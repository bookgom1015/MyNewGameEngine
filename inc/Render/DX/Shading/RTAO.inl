#ifndef __RTAO_INL__
#define __RTAO_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RTAO::RTAOClass::AOCoefficientMap() const {
	return mAOResources[Resource::AO::E_AOCoefficient].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RTAO::RTAOClass::AOCoefficientMapSrv() const {
	return mhAOResourceGpus[Descriptor::AO::ES_AOCoefficient];
}

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RTAO::RTAOClass::AOResource(Resource::AO::Type type) const {
	return mAOResources[type].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RTAO::RTAOClass::AODescriptor(Descriptor::AO::Type type) const {
	return mhAOResourceGpus[type];
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

constexpr UINT Render::DX::Shading::RTAO::RTAOClass::CurrentAOResourceFrameIndex() const {
	return mCurrentAOResourceFrameIndex;
}

#endif // __RTAO_INL__