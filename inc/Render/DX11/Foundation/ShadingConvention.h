#ifndef __SHADINGCONVENTION_H__
#define __SHADINGCONVENTION_H__

namespace ShadingConvention {
	namespace SwapChain {
		const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	namespace GBuffer {
		const DXGI_FORMAT AlbedoMapFormat				= DXGI_FORMAT_R8G8B8A8_UNORM;
		const DXGI_FORMAT NormalMapFormat				= DXGI_FORMAT_R8G8B8A8_SNORM;
		const DXGI_FORMAT PositionMapFormat				= DXGI_FORMAT_R16G16B16A16_FLOAT;
		const DXGI_FORMAT RoughnessMetalnessMapFormat	= DXGI_FORMAT_R16G16_UNORM;
	}
}

#endif // __SHADINGCONVENTION_H__