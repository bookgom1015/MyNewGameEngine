#ifndef __TONEMAPPING_INL__
#define __TONEMAPPING_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::ToneMapping::ToneMappingClass::InterMediateMapResource() { return mIntermediateMap.get(); }

constexpr CD3DX12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::ToneMapping::ToneMappingClass::InterMediateMapSrv() const { return mhIntermediateMapGpuSrv; }

constexpr CD3DX12_CPU_DESCRIPTOR_HANDLE Render::DX::Shading::ToneMapping::ToneMappingClass::InterMediateMapRtv() const { return mhIntermediateMapCpuRtv; }

#endif // __TONEMAPPING_INL__