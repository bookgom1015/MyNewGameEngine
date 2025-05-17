#ifndef __ACCELERATIONSTRUCTURE_INL__
#define __ACCELERATIONSTRUCTURE_INL__

D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Shading::Util::AccelerationStructureManager::AccelerationStructure() const {
	return mTLAS->mResult->GetGPUVirtualAddress();
}

#endif // __ACCELERATIONSTRUCTURE_INL__