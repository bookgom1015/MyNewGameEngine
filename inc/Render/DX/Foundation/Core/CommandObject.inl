#ifndef __COMMANDOBJECT_INL__
#define __COMMANDOBJECT_INL__

ID3D12GraphicsCommandList4* Render::DX::Foundation::Core::CommandObject::DirectCommandList() const {
	return mDirectCommandList.Get();
}

#endif // __COMMANDOBJECT_INL__