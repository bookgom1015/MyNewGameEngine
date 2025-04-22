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
		static const FLOAT InvalidDepthValue = 1.f;
		static const UINT InvalidStencilValue = 0;

#ifdef _HLSL
		typedef float DepthBufferFormat;
#else
		const DXGI_FORMAT DepthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		const DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
#endif
	}

	namespace EnvironmentMap {
		namespace ThreadGroup {
			namespace MeshShader {
				enum {
					ThreadsPerGroup = MESH_SHADER_MAX_PRIMITIVES
				};
			}
		}

#ifndef EnvironmentMap_DrawSkySphere_RCSTRUCT
#define EnvironmentMap_DrawSkySphere_RCSTRUCT {	\
		UINT gVertexCount;						\
		UINT gIndexCount;						\
	};
#endif

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

	namespace GammaCorrection {
#ifndef GammaCorrection_Default_RCSTRUCT
#define GammaCorrection_Default_RCSTRUCT {	\
		FLOAT gGamma;						\
	};
#endif
	#ifndef GammaCorrection_Default_RootConstants
	#define GammaCorrection_Default_RootConstants(reg) cbuffer gRootConstants : register(reg) GammaCorrection_Default_RCSTRUCT
	#endif
#ifdef _HLSL
#else
		namespace RootConstant {
			namespace Default {
				struct Struct GammaCorrection_Default_RCSTRUCT
					enum {
					E_Gamma = 0,
					Count
				};
			}
		}
#endif
	}

	namespace ToneMapping {
	#ifndef ToneMapping_Default_RCSTRUCT
	#define ToneMapping_Default_RCSTRUCT {	\
		FLOAT gExposure;					\
	};
#endif

#ifdef _HLSL
	typedef HDR_FORMAT IntermediateMapFormat;

	#ifndef ToneMapping_Default_RootConstants
		#define ToneMapping_Default_RootConstants(reg) cbuffer gRootConstants : register(reg) ToneMapping_Default_RCSTRUCT
	#endif
#else
	const DXGI_FORMAT IntermediateMapFormat = HDR_FORMAT;

	const FLOAT IntermediateMapClearValues[4] = { 0.f, 0.f, 0.f, 0.f };

	namespace RootConstant {
		namespace Default {
			struct Struct ToneMapping_Default_RCSTRUCT
			enum {
				E_Exposure = 0,
				Count
			};
		}
	}
#endif
	}

	namespace GBuffer {
		static const UINT MaxNumTextures = 32;
		static const FLOAT InvalidVelocityValue = 1000.f;

		namespace ThreadGroup {
			namespace MeshShader {
				enum {
					ThreadsPerGroup = MESH_SHADER_MAX_PRIMITIVES
				};
			}
		}

#ifndef GBuffer_Default_RCSTRUCT
#define GBuffer_Default_RCSTRUCT {	\
		UINT gVertexCount;			\
		UINT gIndexCount;			\
	};
#endif

#ifdef _HLSL
		typedef float4								AlbedoMapFormat;
		typedef float4								NormalMapFormat;
		typedef float4								RMSMapFormat;
		typedef float2								VelocityMapFormat;
		typedef float4								PositionMapFormat;

#ifndef GBuffer_Default_RootConstants
#define GBuffer_Default_RootConstants(reg) cbuffer cbRootConstant : register(reg) GBuffer_Default_RCSTRUCT
#endif
#else 
		static const DXGI_FORMAT AlbedoMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		static const DXGI_FORMAT NormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		static const DXGI_FORMAT RMSMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		static const DXGI_FORMAT VelocityMapFormat = DXGI_FORMAT_R16G16_FLOAT;
		static const DXGI_FORMAT PositionMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

		const FLOAT AlbedoMapClearValues[4] = { 0.f,  0.f, 0.f,  0.f };
		const FLOAT NormalMapClearValues[4] = { 0.f,  0.f, 0.f, -1.f };
		const FLOAT RMSMapClearValues[4] = { 0.5f, 0.f, 0.5f, 0.f };
		const FLOAT VelocityMapClearValues[2] = { InvalidVelocityValue, InvalidVelocityValue };
		const FLOAT PositionMapClearValues[4] = { 0.f, 0.f, 0.f, -1.f };

		namespace RootConstant {
			namespace Default {
				struct Struct GBuffer_Default_RCSTRUCT
					enum {
					EC_VertexCount = 0,
					EC_IndexCount,
					Count
				};
			}
		}
#endif 
	}
}

#endif // __SHADINGCONVENTION_H__