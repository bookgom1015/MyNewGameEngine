#ifndef __SHADINGCONVENTION_H__
#define __SHADINGCONVENTION_H__

namespace ShadingConvention {
	namespace SwapChain {
		const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	namespace GBuffer {
		const DXGI_FORMAT AlbedoMapFormat				= DXGI_FORMAT_R8G8B8A8_UNORM;
		const DXGI_FORMAT NormalMapFormat				= DXGI_FORMAT_R16G16B16A16_FLOAT;
		const DXGI_FORMAT PositionMapFormat				= DXGI_FORMAT_R16G16B16A16_FLOAT;
		const DXGI_FORMAT RoughnessMetalnessMapFormat	= DXGI_FORMAT_R16G16_UNORM;
		const DXGI_FORMAT VelocityMapFormat				= DXGI_FORMAT_R16G16_FLOAT;

		const FLOAT AlbedoMapClearValues[4]{ 0.f, 0.f, 0.f, 0.f };		
		const FLOAT NormalMapClearValues[4]{ 0.f, 0.f, 0.f, -1.f };
		const FLOAT PositionMapClearValues[4]{ 0.f, 0.f, 0.f, -1.f };
		const FLOAT RoughnessMapClearValues[4]{ 0.f, 0.f, 0.f, 0.f };
		const FLOAT VelocityMapClearValues[4] = { 1000.f, 1000.f, 0.f, 0.f };
	}

	namespace DepthStencilBuffer {
		const DXGI_FORMAT DepthStencilBufferFormat = DXGI_FORMAT_R24G8_TYPELESS;
		const DXGI_FORMAT DepthStencilViewFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		const DXGI_FORMAT ShaderResourceViewFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}

	namespace Shadow {
		const DXGI_FORMAT ZDepthMapFormat = DXGI_FORMAT_R32_TYPELESS;
		const DXGI_FORMAT ShadowMapFormat = DXGI_FORMAT_R32_UINT;

		static const UINT ShadowMapType_Texture2D		= 0;
		static const UINT ShadowMapType_Texture2DArray	= 1;
		static const UINT ShadowMapType_CubeMap			= 2;		
	}

	namespace ToneMapping {
		const DXGI_FORMAT IntermediateMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	}

	namespace TAA{
		const DXGI_FORMAT HistoryMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
}

#endif // __SHADINGCONVENTION_H__