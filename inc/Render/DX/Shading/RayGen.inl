#ifndef __RAYGEN_INL__
#define __RAYGEN_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RayGen::RayGenClass::RayDirectionOriginDepthMap() const {
	return mRayDirectionOriginDepthMap.get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RayGen::RayGenClass::RayDirectionOriginDepthMapSrv() const {
	return mhRayDirectionOriginDepthMapGpuSrv;
}

#endif // __RAYGEN_INL__