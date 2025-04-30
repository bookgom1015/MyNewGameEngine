#ifndef __TONEMAPPING_INL__
#define __TONEMAPPING_INL__

// InterMediateMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::ToneMapping::ToneMappingClass::InterMediateMapResource() { 
	return mIntermediateMap.get(); 
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::ToneMapping::ToneMappingClass::InterMediateMapSrv() const { 
	return mhIntermediateMapGpuSrv; 
}

constexpr D3D12_CPU_DESCRIPTOR_HANDLE Render::DX::Shading::ToneMapping::ToneMappingClass::InterMediateMapRtv() const { 
	return mhIntermediateMapCpuRtv; 
}

// InterMediateCopyMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::ToneMapping::ToneMappingClass::InterMediateCopyMapResource() {
	return mIntermediateCopyMap.get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::ToneMapping::ToneMappingClass::InterMediateCopyMapSrv() const {
	return mhIntermediateCopyMapGpuSrv;
}

#endif // __TONEMAPPING_INL__