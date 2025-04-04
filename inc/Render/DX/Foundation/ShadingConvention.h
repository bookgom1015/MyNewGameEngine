#ifndef __SHADINGCONVENTION_H__
#define __SHADINGCONVENTION_H__

#ifdef _HLSL
	#ifndef HDR_FORMAT
		#define HDR_FORMAT float4
	#endif
#else
	#ifndef HDR_FORMAT
		#define HDR_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
	#endif
#endif

namespace ShadingConvention{
	namespace SwapChain {
#ifdef _HLSL
		typedef float4 BackBufferFormat;
#else
		const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
#endif
	}

	namespace DepthStencilBuffer {
#ifdef _HLSL
		typedef float DepthStencilBufferFormat;
#else
		const DXGI_FORMAT DepthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
#endif
	}

	namespace EnvironmentMap {
#ifdef _HLSL
		typedef HDR_FORMAT EnvironmentCubeMap;
#else
		const DXGI_FORMAT EnvironmentCubeMap = HDR_FORMAT;
#endif
	}

	namespace MipmapGenerator {
#ifndef MipmapGenerator_Default_RCSTRUCT
#define MipmapGenerator_Default_RCSTRUCT {		\
		DirectX::XMFLOAT2 gInvTexSize;			\
		DirectX::XMFLOAT2 gInvMipmapTexSize;	\
	};
#endif

#ifdef _HLSL
	#ifndef MipmapGenerator_Default_RootConstants
	#define MipmapGenerator_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) MipmapGenerator_Default_RCSTRUCT
	#endif
#else
		namespace RootConstant {
			namespace Default {
				struct Struct MipmapGenerator_Default_RCSTRUCT
					enum {
					E_InvTexSizeW = 0,
					E_InvTexSizeH,
					E_InvMipmapTexSizeW,
					E_InvMipmapTexSizeH,
					Count
				};
			}
		}
#endif
	}
}

#endif // __SHADINGCONVENTION_H__