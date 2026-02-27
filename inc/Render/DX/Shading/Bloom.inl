#ifndef __BLOOM_INL__
#define __BLOOM_INL__

namespace Render::DX::Shading::Bloom {
	Foundation::Resource::GpuResource* BloomClass::BloomMap() const {
		return mBloomMaps[0].get();
	}

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE BloomClass::BloomMapSrv() const {
		return mhBloomMapGpuSrvs[0];
	}
}

#endif // __BLOOM_INL__