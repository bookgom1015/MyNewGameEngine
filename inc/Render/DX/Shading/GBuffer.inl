#ifndef __GBUFFER_INL__
#define __GBUFFER_INL__

namespace Render::DX::Shading::GBuffer {
	// AlbedoMap
	Foundation::Resource::GpuResource* GBufferClass::AlbedoMap() const { 
		return mResources[Resource::E_Albedo].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::AlbedoMapSrv() const { 
		return mhGpuSrvs[Descriptor::Srv::E_Albedo]; }

	// NormalMap
	Foundation::Resource::GpuResource* GBufferClass::NormalMap() const { 
		return mResources[Resource::E_Normal].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::NormalMapSrv() const { 
		return mhGpuSrvs[Descriptor::Srv::E_Normal]; }

	// NormalDepthMap
	Foundation::Resource::GpuResource* GBufferClass::NormalDepthMap() const { 
		return mResources[Resource::E_NormalDepth].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::NormalDepthMapSrv() const { 
		return mhGpuSrvs[Descriptor::Srv::E_NormalDepth]; }

	// ReprojectedNormalDepthMap
	Foundation::Resource::GpuResource* GBufferClass::ReprojectedNormalDepthMap() const { 
		return mResources[Resource::E_ReprojNormalDepth].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::ReprojectedNormalDepthMapSrv() const { 
		return mhGpuSrvs[Descriptor::Srv::E_ReprojNormalDepth]; }

	// CachedNormalDepthMap
	Foundation::Resource::GpuResource* GBufferClass::CachedNormalDepthMap() const { 
		return mResources[Resource::E_CachedNormalDepth].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::CachedNormalDepthMapSrv() const {
		return mhGpuSrvs[Descriptor::Srv::E_CachedNormalDepth]; }

	// SpecularMap
	Foundation::Resource::GpuResource* GBufferClass::SpecularMap() const { 
		return mResources[Resource::E_Specular].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::SpecularMapSrv() const { 
		return mhGpuSrvs[Descriptor::Srv::E_Specular]; }

	// RoughnessMetalnessMap
	Foundation::Resource::GpuResource* GBufferClass::RoughnessMetalnessMap() const { 
		return mResources[Resource::E_RoughnessMetalness].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::RoughnessMetalnessMapSrv() const {
		return mhGpuSrvs[Descriptor::Srv::E_RoughnessMetalness]; }

	// VelocityMap
	Foundation::Resource::GpuResource* GBufferClass::VelocityMap() const { 
		return mResources[Resource::E_Velcity].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::VelocityMapSrv() const { 
		return mhGpuSrvs[Descriptor::Srv::E_Velocity]; }

	// PositionMap
	Foundation::Resource::GpuResource* GBufferClass::PositionMap() const { 
		return mResources[Resource::E_Position].get(); }

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE GBufferClass::PositionMapSrv() const { 
		return mhGpuSrvs[Descriptor::Srv::E_Position]; }
}

#endif // __GBUFFER_INL__