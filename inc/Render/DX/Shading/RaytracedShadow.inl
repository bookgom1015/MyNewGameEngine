#ifndef __RAYTRACEDSHADOW_INL__
#define __RAYTRACEDSHADOW_INL__

namespace Render::DX::Shading::RaytracedShadow {
	Foundation::Resource::GpuResource* RaytracedShadowClass::ShadowMap() const {
		return mShadowMap.get();
	}

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE RaytracedShadowClass::ShadowMapSrv() const {
		return mhShadowMapGpuSrv;
	}
}

#endif // __RAYTRACEDSHADOW_INL__