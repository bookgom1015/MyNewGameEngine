#ifndef __RAYSORTING_INL__
#define __RAYSORTING_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RaySorting::RaySortingClass::RayIndexOffsetMap() const {
	return mRayIndexOffsetMap.get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RaySorting::RaySortingClass::RayIndexOffsetMapSrv() const {
	return mhRayIndexOffsetMapGpuSrv;
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RaySorting::RaySortingClass::RayIndexOffsetMapUav() const {
	return mhRayIndexOffsetMapGpuUav;
}

#endif // __RAYSORTING_INL__