#ifndef __FRAMERESOURCE_INL__
#define __FRAMERESOURCE_INL__

ID3D12CommandAllocator* Render::DX::Foundation::Resource::FrameResource::CommandAllocator(UINT index) const {
	return mCmdAllocators[index].Get();
}

void Render::DX::Foundation::Resource::FrameResource::CommandAllocators(std::vector<ID3D12CommandAllocator*>& allocs) const {
	for (UINT i = 0; i < mThreadCount; ++i)
		allocs.push_back(mCmdAllocators[i].Get());
}

#endif // __FRAMERESOURCE_INL__