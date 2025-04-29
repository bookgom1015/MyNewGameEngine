#ifndef __ENVIRONMENTMAP_INL__
#define __ENVIRONMENTMAP_INL__


// DiffuseIrradianceMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::EnvironmentMap::EnvironmentMapClass::DiffuseIrradianceCubeMap() const { return mDiffuseIrradianceCubeMap.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::EnvironmentMap::EnvironmentMapClass::DiffuseIrradianceCubeMapSrv() const {	return mhDiffuseIrradianceCubeMapGpuSrv; }

// PrefilteredEnvironmentCubeMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::EnvironmentMap::EnvironmentMapClass::PrefilteredEnvironmentCubeMap() const { return mPrefilteredEnvironmentCubeMap.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::EnvironmentMap::EnvironmentMapClass::PrefilteredEnvironmentCubeMapSrv() const { return mhPrefilteredEnvironmentCubeMapGpuSrv; }

// BrdfLutMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::EnvironmentMap::EnvironmentMapClass::BrdfLutMap() const { return mBrdfLutMap.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::EnvironmentMap::EnvironmentMapClass::BrdfLutMapSrv() const { return mhBrdfLutMapGpuSrv; }

#endif // __ENVIRONMENTMAP_INL__