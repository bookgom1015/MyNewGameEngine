#ifndef __DEVICE_INL__
#define __DEVICE_INL__

ID3D12Device5* Render::DX::Foundation::Core::Device::GetDevice() const {
	return md3dDevice.Get();
}

#endif // __DEVICE_INL__