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

// ObjectCB
D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::ObjectCBAddress() const {
	return mObjectCB.Resource()->GetGPUVirtualAddress();
}

void Render::DX::Foundation::Resource::FrameResource::CopyObjecCB(INT elementIndex, const ConstantBuffers::ObjectCB& data) {
	mObjectCB.CopyData(elementIndex, data);
}

// EquirectangularConverterCB
D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::FrameResource::EquirectConvCBAddress() const {
	return mEquirectConvCB.Resource()->GetGPUVirtualAddress();
}

void Render::DX::Foundation::Resource::FrameResource::CopyEquirectConvCB(INT elementIndex, const ConstantBuffers::EquirectangularConverterCB& data) {
	mEquirectConvCB.CopyData(elementIndex, data);
}

#endif // __FRAMERESOURCE_INL__