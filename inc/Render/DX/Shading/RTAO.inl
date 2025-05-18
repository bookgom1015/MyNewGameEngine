#ifndef __RTAO_INL__
#define __RTAO_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RTAO::RTAOClass::AOCoefficientMap() const {
	return mAOResources[Resource::AO::E_AOCoefficient].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RTAO::RTAOClass::AOCoefficientMapSrv() const {
	return mhAOResourceGpus[Descriptor::AO::ES_AOCoefficient];
}

#endif // __RTAO_INL__