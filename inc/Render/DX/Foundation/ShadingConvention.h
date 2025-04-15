#ifndef __SHADINGCONVENTION_H__
#define __SHADINGCONVENTION_H__

#ifdef _HLSL
	#ifndef HDR_FORMAT
	#define HDR_FORMAT float4
	#endif

	#ifndef SDR_FORMAT
	#define SDR_FORMAT float4
	#endif
#else
	#ifndef HDR_FORMAT
	#define HDR_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
	#endif

	#ifndef SDR_FORMAT
	#define SDR_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM
	#endif
#endif

#ifndef MESH_SHADER_MAX_VERTICES
#define MESH_SHADER_MAX_VERTICES 192
#endif

#ifndef MESH_SHADER_MAX_PRIMITIVES
#define MESH_SHADER_MAX_PRIMITIVES 64
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
#ifndef EnvironmentMap_DrawSkySphere_RCSTRUCT
#define EnvironmentMap_DrawSkySphere_RCSTRUCT {	\
		UINT gVertexCount;						\
		UINT gIndexCount;						\
	};
#endif

		namespace ThreadGroup {
			namespace MeshShader {
				enum {
					ThreadsPerGroup = MESH_SHADER_MAX_PRIMITIVES
				};
			}
		}

#ifdef _HLSL
		typedef HDR_FORMAT EquirectangularMapFormat;
		typedef HDR_FORMAT EnvironmentCubeMapFormat;

	#ifndef EnvironmentMap_DrawSkySphere_RootConstants
	#define EnvironmentMap_DrawSkySphere_RootConstants(reg) cbuffer cbRootConstants : register(reg) EnvironmentMap_DrawSkySphere_RCSTRUCT
	#endif
#else
		const DXGI_FORMAT EquirectangularMapFormat = HDR_FORMAT;
		const DXGI_FORMAT EnvironmentCubeMapFormat = HDR_FORMAT;

		namespace RootConstant {
			namespace DrawSkySphere {
				struct Struct EnvironmentMap_DrawSkySphere_RCSTRUCT
					enum {
					E_VertexCount = 0,
					E_IndexCount,
					Count
				};
			}
		}
#endif
	}

	namespace MipmapGenerator {
		static const UINT MaxMipLevel = 5;

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

	namespace EquirectangularConverter {
#ifndef EquirectangularConverter_ConvCubeToEquirect_RCSTRUCT
#define EquirectangularConverter_ConvCubeToEquirect_RCSTRUCT {	\
		UINT gMipLevel;											\
	};
#endif
	#ifndef EquirectangularConverter_ConvCubeToEquirect_RootConstants
	#define EquirectangularConverter_ConvCubeToEquirect_RootConstants(reg) cbuffer cbRootConstants : register(reg) EquirectangularConverter_ConvCubeToEquirect_RCSTRUCT
	#endif
#ifdef _HLSL
#else
		namespace RootConstant {
			namespace ConvCubeToEquirect {
				struct Struct EquirectangularConverter_ConvCubeToEquirect_RCSTRUCT
					enum {
					E_MipLevel = 0,
					Count
				};
			}
		}
#endif
	}
}

#endif // __SHADINGCONVENTION_H__