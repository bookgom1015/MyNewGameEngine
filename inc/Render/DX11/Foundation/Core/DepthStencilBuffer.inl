#ifndef __DEPTHSTENCILBUFFER_INL__
#define __DEPTHSTENCILBUFFER_INL__

namespace Render::DX11::Foundation::Core {
	ID3D11DepthStencilView* DepthStencilBuffer::DepthStencilView() noexcept {
		return mhDepthStencilBufferView.Get();
	}

	ID3D11ShaderResourceView* DepthStencilBuffer::DepthMapSrv() noexcept {
		return mhDepthMapSrv.Get();
	}
}

#endif // __DEPTHSTENCILBUFFER_INL__