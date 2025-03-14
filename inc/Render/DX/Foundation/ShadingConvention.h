#ifndef __SHADINGCONVENTION_H__
#define __SHADINGCONVENTION_H__

namespace ShadingConvention{
	namespace SwapChain {
		const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	namespace DepthStencilBuffer {
		const DXGI_FORMAT DepthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
}

#endif // __SHADINGCONVENTION_H__