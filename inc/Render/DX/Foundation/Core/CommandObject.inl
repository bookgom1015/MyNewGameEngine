#ifndef __COMMANDOBJECT_INL__
#define __COMMANDOBJECT_INL__

ID3D12GraphicsCommandList6* Render::DX::Foundation::Core::CommandObject::DirectCommandList() const {
	return mDirectCommandList.Get();
}

ID3D12GraphicsCommandList6* Render::DX::Foundation::Core::CommandObject::CommandList(UINT index) const {
	return mMultiCommandLists[index].Get();
}

constexpr UINT64 Render::DX::Foundation::Core::CommandObject::CurrentFence() const noexcept {
	return mCurrentFence;
}

#endif // __COMMANDOBJECT_INL__