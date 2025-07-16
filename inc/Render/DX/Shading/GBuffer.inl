#ifndef __GBUFFER_INL__
#define __GBUFFER_INL__

// AlbedoMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::AlbedoMap() const { return mAlbedoMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::AlbedoMapSrv() const { return mhAlbedoMapGpuSrv; }

// NormalMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::NormalMap() const { return mNormalMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::NormalMapSrv() const { return mhNormalMapGpuSrv; }

// NormalDepthMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::NormalDepthMap() const { return mNormalDepthMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::NormalDepthMapSrv() const { return mhNormalDepthMapGpuSrv; }

// ReprojectedNormalDepthMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::ReprojectedNormalDepthMap() const { return mReprojNormalDepthMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::ReprojectedNormalDepthMapSrv() const { return mhReprojNormalDepthMapGpuSrv; }

// CachedNormalDepthMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::CachedNormalDepthMap() const { return mCachedNormalDepthMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::CachedNormalDepthMapSrv() const { return mhCachedNormalDepthMapGpuSrv; }

// SpecularMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::SpecularMap() const { return mSpecularMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::SpecularMapSrv() const { return mhSpecularMapGpuSrv; }

// RoughnessMetalnessMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::RoughnessMetalnessMap() const { return mRoughnessMetalnessMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::RoughnessMetalnessMapSrv() const { return mhRoughnessMetalnessMapGpuSrv; }

// VelocityMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::VelocityMap() const { return mVelocityMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::VelocityMapSrv() const { return mhVelocityMapGpuSrv; }

// PositionMap
Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::GBuffer::GBufferClass::PositionMap() const { return mPositionMap.get(); }

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::GBuffer::GBufferClass::PositionMapSrv() const { return mhPositionMapGpuSrv; }

#endif // __GBUFFER_INL__