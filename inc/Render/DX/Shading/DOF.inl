#ifndef __DOF_INL__
#define __DOF_INL__

namespace Render::DX::Shading::DOF {
	Foundation::Resource::GpuResource* DOFClass::CoCMap() const {
		return mCircleOfConfusionMap.get();
	}

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE DOFClass::CoCMapSrv() const {
		return mhCircleOfConfusionMapGpuSrv;
	}
}

#endif // __DOF_INL__