#ifndef __FRAMERESOURCE_INL__
#define __FRAMERESOURCE_INL__

ID3D12CommandAllocator* Render::DX::Foundation::Resource::FrameResource::CommandAllocator(UINT index) const {
	return mCmdAllocators[index].Get();
}

void Render::DX::Foundation::Resource::FrameResource::CommandAllocators(std::vector<ID3D12CommandAllocator*>& allocs) const {
	for (UINT i = 0; i < mThreadCount; ++i)
		allocs.push_back(mCmdAllocators[i].Get());
}

// MainPassCB
D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::MainPassCBAddress() const {
	return mMainPassCB.Resource()->GetGPUVirtualAddress();
}

void Render::DX::Foundation::Resource::FrameResource::CopyMainPassCB(INT elementIndex, const ConstantBuffers::PassCB& data) {
	mMainPassCB.CopyData(elementIndex, data);
}

D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::LightCBAddress() const {
	return mLightCB.Resource()->GetGPUVirtualAddress();
}

void Render::DX::Foundation::Resource::FrameResource::CopyLightCB(INT elementIndex, const ConstantBuffers::LightCB& data) {
	mLightCB.CopyData(elementIndex, data);
}

// ObjectCB
D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::ObjectCBAddress() const {
	return mObjectCB.Resource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::ObjectCBAddress(UINT index) const {
	return mObjectCB.Resource()->GetGPUVirtualAddress() + static_cast<UINT64>(index) * static_cast<UINT64>(ObjectCBByteSize());
}

void Render::DX::Foundation::Resource::FrameResource::CopyObjectCB(INT elementIndex, const ConstantBuffers::ObjectCB& data) {
	mObjectCB.CopyData(elementIndex, data);
}

// MaterialCB
D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::MaterialCBAddress() const {
	return mMaterialCB.Resource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::MaterialCBAddress(UINT index) const {
	return mMaterialCB.Resource()->GetGPUVirtualAddress() + static_cast<UINT64>(index) * static_cast<UINT64>(MaterialCBByteSize());
}

void Render::DX::Foundation::Resource::FrameResource::CopyMaterialCB(INT elementIndex, const ConstantBuffers::MaterialCB& data) {
	mMaterialCB.CopyData(elementIndex, data);
}

// ProjectToCubeCB
D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::ProjectToCubeCBAddress() const {
	return mProjectToCubeCB.Resource()->GetGPUVirtualAddress();
}

void Render::DX::Foundation::Resource::FrameResource::CopyProjectToCubeCB(const ConstantBuffers::ProjectToCubeCB& data) {
	mProjectToCubeCB.CopyData(0, data);
}

// AmbientOcclusionCB
D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::AmbientOcclusionCBAddress() const {
	return mAmbientOcclusionCB.Resource()->GetGPUVirtualAddress();
}

void Render::DX::Foundation::Resource::FrameResource::CopyAmbientOcclusionCB(const ConstantBuffers::AmbientOcclusionCB& data) {
	mAmbientOcclusionCB.CopyData(0, data);
}

#endif // __FRAMERESOURCE_INL__