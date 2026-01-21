#ifndef __DEVICE_INL__
#define __DEVICE_INL__

namespace Render::DX11::Foundation::Core {
	ID3D11DeviceContext1* Device::Context() noexcept { return mContext.Get(); }
}

#endif // __DEVICE_INL__