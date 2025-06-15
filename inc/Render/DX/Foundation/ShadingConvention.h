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

#ifndef EnvironmentMap_ConvoluteSpecularIrradiance_RCSTRUCT
#define EnvironmentMap_ConvoluteSpecularIrradiance_RCSTRUCT {	\
		UINT	gMipLevel;										\
		FLOAT	gRoughness;										\
		FLOAT	gResolution;									\
	};
#endif

#ifdef _HLSL
		typedef HDR_FORMAT EquirectangularMapFormat;
		typedef HDR_FORMAT EnvironmentCubeMapFormat;
		typedef HDR_FORMAT DiffuseIrradianceCubeMapFormat;
		typedef HDR_FORMAT PrefilteredEnvironmentCubeMapFormat;
		typedef float2 BrdfLutMapFormat;

	#ifndef EnvironmentMap_DrawSkySphere_RootConstants
	#define EnvironmentMap_DrawSkySphere_RootConstants(reg) cbuffer cbRootConstants : register(reg) EnvironmentMap_DrawSkySphere_RCSTRUCT
	#endif

	#ifndef EnvironmentMap_ConvoluteSpecularIrradiance_RootConstants
	#define EnvironmentMap_ConvoluteSpecularIrradiance_RootConstants(reg) cbuffer cbRootConstants : register(reg) EnvironmentMap_ConvoluteSpecularIrradiance_RCSTRUCT
	#endif
#else
		const DXGI_FORMAT EquirectangularMapFormat = HDR_FORMAT;
		const DXGI_FORMAT EnvironmentCubeMapFormat = HDR_FORMAT;
		const DXGI_FORMAT DiffuseIrradianceCubeMapFormat = HDR_FORMAT;
		const DXGI_FORMAT PrefilteredEnvironmentCubeMapFormat = HDR_FORMAT;
		const DXGI_FORMAT BrdfLutMapFormat = DXGI_FORMAT_R16G16_FLOAT;

		namespace RootConstant {
			namespace DrawSkySphere {
				struct Struct EnvironmentMap_DrawSkySphere_RCSTRUCT
					enum {
					E_VertexCount = 0,
					E_IndexCount,
					Count
				};
			}

			namespace ConvoluteSpecularIrradiance {
				struct Struct EnvironmentMap_ConvoluteSpecularIrradiance_RCSTRUCT
				enum {
					E_MipLevel = 0,
					E_Roughness,
					E_Resolution,
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

		static const FLOAT	InvalidNormalWValue		= -1.f;
		static const UINT	InvalidNormalDepthValue	= 0;
		static const FLOAT	InvalidVelocityValue	= 1000.f;
		static const FLOAT	InvalidPositionWValue	= -1.f;

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
		typedef float4	AlbedoMapFormat;
		typedef float4	NormalMapFormat;
		typedef uint	NormalDepthMapFormat;
		typedef float4	SpecularMapFormat;
		typedef float2	RoughnessMetalnessMapFormat;
		typedef float2	VelocityMapFormat;
		typedef float4	PositionMapFormat;

		bool IsValidNormal(float4 normal) {
			return normal.w != InvalidNormalWValue;
		}

		bool IsValidNormalDepth(uint normalDepth) {
			return normalDepth != InvalidNormalDepthValue;
		}

		bool IsValidPosition(float4 position) {
			return position.w != InvalidPositionWValue;
		}

		bool IsValidVelocity(float2 velocity) {
			return all(velocity != InvalidVelocityValue);
		}

#ifndef GBuffer_Default_RootConstants
#define GBuffer_Default_RootConstants(reg) cbuffer cbRootConstant : register(reg) GBuffer_Default_RCSTRUCT
#endif
#else 
		const DXGI_FORMAT AlbedoMapFormat				= DXGI_FORMAT_R8G8B8A8_UNORM;
		const DXGI_FORMAT NormalMapFormat				= DXGI_FORMAT_R16G16B16A16_FLOAT;
		const DXGI_FORMAT NormalDepthMapFormat			= DXGI_FORMAT_R32_UINT;
		const DXGI_FORMAT SpecularMapFormat				= DXGI_FORMAT_R8G8B8A8_UNORM;
		const DXGI_FORMAT RoughnessMetalnessMapFormat	= DXGI_FORMAT_R16G16_UNORM;
		const DXGI_FORMAT VelocityMapFormat				= DXGI_FORMAT_R16G16_FLOAT;
		const DXGI_FORMAT PositionMapFormat				= DXGI_FORMAT_R16G16B16A16_FLOAT;

		const FLOAT AlbedoMapClearValues[4]				= { 0.f,  0.f, 0.f,  0.f };
		const FLOAT NormalMapClearValues[4]				= { 0.f,  0.f, 0.f, InvalidNormalWValue };
		const FLOAT NormalDepthMapClearValues[4]		= { 0.f,  0.f, 0.f, 0.f };
		const FLOAT SpecularMapClearValues[4]			= { 0.08f, 0.08f, 0.08f, 0.f };
		const FLOAT RoughnessMetalnessMapClearValues[2] = { 0.5f, 0.f };
		const FLOAT VelocityMapClearValues[2]			= { InvalidVelocityValue, InvalidVelocityValue };
		const FLOAT PositionMapClearValues[4]			= { 0.f, 0.f, 0.f, InvalidPositionWValue };

		namespace RootConstant {
			namespace Default {
				struct Struct GBuffer_Default_RCSTRUCT
					enum {
					E_VertexCount = 0,
					E_IndexCount,
					Count
				};
			}
		}
#endif 
	}

	namespace BRDF {
#ifndef BRDF_ComputeBRDF_RCSTRUCT
#define BRDF_ComputeBRDF_RCSTRUCT {	\
		BOOL gShadowEnabled;		\
	};
#endif

#ifndef BRDF_IntegrateIrradiance_RCSTRUCT
#define BRDF_IntegrateIrradiance_RCSTRUCT {	\
		BOOL gAoEnabled;					\
	};
#endif

#ifdef _HLSL
#ifndef BRDF_ComputeBRDF_RootConstants
#define BRDF_ComputeBRDF_RootConstants(reg) cbuffer cbRootConstant : register(reg) BRDF_ComputeBRDF_RCSTRUCT
#endif

#ifndef BRDF_IntegrateIrradiance_RootConstants
#define BRDF_IntegrateIrradiance_RootConstants(reg) cbuffer cbRootConstant : register(reg) BRDF_IntegrateIrradiance_RCSTRUCT
#endif
#else
		namespace RootConstant {
			namespace ComputeBRDF {
				struct Struct BRDF_ComputeBRDF_RCSTRUCT
					enum {
					E_ShadowEnabled = 0,
					Count
				};
			}

			namespace IntegrateIrradiance {
				struct Struct BRDF_IntegrateIrradiance_RCSTRUCT
					enum {
					E_SsaoEnabled = 0,
					Count
				};
			}
		}
#endif
	}

	namespace Shadow {
		namespace ThreadGroup {
			namespace DrawShadow {
				enum {
					Width = 8,
					Height = 8,
					Depth = 1,
					Size = Width * Height * Depth
				};
			}
		}

#ifndef Shadow_DrawZDepth_RCSTRUCT
#define Shadow_DrawZDepth_RCSTRUCT {	\
		UINT gLightIndex;				\
	};
#endif

#ifndef Shadow_DrawShadow_RCSTRUCT			
#define Shadow_DrawShadow_RCSTRUCT {	\
		UINT gLightIndex;				\
	};
#endif

#ifdef _HLSL
		typedef float	ZDepthMapFormat;
		typedef uint	ShadowMapFormat;
		typedef float4	DebugMapFormat;

		static const uint InvalidFaceId = 255;

		bool IsValidFaceId(uint faceId) {
			return faceId != InvalidFaceId;
		}

#ifndef Shadow_DrawZDepth_RootConstants
#define Shadow_DrawZDepth_RootConstants(reg) cbuffer cbRootConstants : register(reg) Shadow_DrawZDepth_RCSTRUCT
#endif

#ifndef Shadow_DrawShadow_RootConstants
#define Shadow_DrawShadow_RootConstants(reg) cbuffer cbRootConstants : register(reg) Shadow_DrawShadow_RCSTRUCT
#endif
#else 
		const DXGI_FORMAT ZDepthMapFormat = DXGI_FORMAT_D32_FLOAT;
		const DXGI_FORMAT ShadowMapFormat = DXGI_FORMAT_R16_UINT;
		const DXGI_FORMAT DebugMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

		const FLOAT FaceIdCubeMapClearValues[4] = { 255.f, 0.f, 0.f, 0.f };
#endif 
		namespace RootConstant {
			namespace DrawZDepth {
				struct Struct Shadow_DrawZDepth_RCSTRUCT
					enum {
					E_LightIndex = 0,
					Count
				};
			}

			namespace DrawShadow {
				struct Struct Shadow_DrawShadow_RCSTRUCT
					enum {
					E_LightIndex = 0,
					Count
				};
			}
		}
	}

	namespace TAA {
#ifndef TAA_Default_RCSTRUCT
#define TAA_Default_RCSTRUCT {					\
		FLOAT			  gModulationFactor;	\
		DirectX::XMFLOAT2 gInvTexDim;			\
	};
#endif

#ifdef _HLSL
#ifndef TAA_Default_RootConstants
#define TAA_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) TAA_Default_RCSTRUCT
#endif
#else
		namespace RootConstant {
			namespace Default {
				struct Struct TAA_Default_RCSTRUCT
				enum {
					E_ModulationFactor = 0,
					E_InvTexDimX,
					E_InvTexDimY,
					Count
				};
			}
		}
#endif
	}

	namespace SSAO {
#ifndef SSAO_Default_RCSTRUCT
#define SSAO_Default_RCSTRUCT {			\
		DirectX::XMFLOAT2 gInvTexDim;	\
	};
#endif

		namespace ThreadGroup {
			namespace Default {
				enum {
					Width = 8,
					Height = 8,
					Depth = 1,
					Size = Width * Height * Depth
				};
			}
		}

#ifdef _HLSL
		typedef float	AOMapFormat;
		typedef float3	RandomVectorMapFormat;

#ifndef SSAO_Default_RootConstants
#define SSAO_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) SSAO_Default_RCSTRUCT
#endif
#else
		const DXGI_FORMAT AOMapFormat = DXGI_FORMAT_R16_UNORM;
		const DXGI_FORMAT RandomVectorMapFormat = DXGI_FORMAT_R8G8B8A8_SNORM;

		const FLOAT AOMapClearValues[4] = { 1.f, 0.f, 0.f, 0.f };

		namespace RootConstant {
			namespace Default {
				struct Struct SSAO_Default_RCSTRUCT
					enum {
					E_InvTexDimX = 0,
					E_InvTexDimY,
					Count
				};
			}
		}
#endif
	}

	namespace RayGen {
		struct AlignedUnitSquareSample2D {
			DirectX::XMFLOAT2	Value;
			DirectX::XMUINT2	padding;  // Padding to 16B
		};

		struct AlignedHemisphereSample3D {
			DirectX::XMFLOAT3	Value;
			UINT				__Padding__;  // Padding to 16B
		};

		namespace ThreadGroup {
			namespace Default {
				enum {
					Width = 8,
					Height = 8,
					Depth = 1,
					Size = Width * Height * Depth
				};
			}
		}
	}

	namespace RaySorting {
		namespace ThreadGroup {
			enum { 
				Width	= 64, 
				Height	= 16, 
				Depth	= 1,
				Size	= Width * Height * Depth
			};
		}

		namespace RayGroup {
			enum { 
				NumElementPairsPerThread	= 4, 
				Width						= ThreadGroup::Width, 
				Height						= NumElementPairsPerThread * 2 * ThreadGroup::Height, 
				Size						= Width * Height 
			};
		}

#ifdef _HLSL
		
#else
		static_assert(
			RayGroup::Width <= 64 &&
			RayGroup::Height <= 128 &&
			RayGroup::Size <= 8192,
			"Ray group dimensions are outside the supported limits set by the Counting Sort shader.");
#endif
	}

	namespace SVGF {
		namespace ThreadGroup {
			namespace Default {
				enum {
					Width = 8,
					Height = 8,
					Depth = 1,
					Size = Width * Height * Depth
				};
			}

			namespace Atrous {
				enum {
					Width = 16,
					Height = 16,
					Depth = 1,
					Size = Width * Height * Depth
				};
			}
		}

#ifndef SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT
#define SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT {	\
		DirectX::XMFLOAT2 gTexDim;								\
		DirectX::XMFLOAT2 gInvTexDim;							\
	};
#endif

#ifndef SVGF_CalcDepthPartialDerivative_RCSTRUCT
#define SVGF_CalcDepthPartialDerivative_RCSTRUCT {	\
		DirectX::XMFLOAT2 gInvTexDim;				\
	};
#endif

#ifdef _HLSL
		typedef float		ValueMapFormat_Contrast;
		typedef HDR_FORMAT	ValueMapFormat_Color;
		typedef float		ValueSquaredMeanMapFormat_Contrast;
		typedef HDR_FORMAT	ValueSquaredMeanMapFormat_Color;

		typedef uint4	TSPPSquaredMeanRayHitDistanceMapFormat;
		typedef float2	DepthPartialDerivativeMapFormat;
		typedef float2	LocalMeanVarianceMapFormat;
		typedef float	VarianceMapFormat;
		typedef float	RayHitDistanceFormat;
		typedef uint	TSPPMapFormat;
		typedef float	DisocclusionBlurStrengthMapFormat;

		static const float InvalidContrastValue = -1.f;
		static const float InvalidColorValueW = -1.f;

		bool IsValidColorValue(float4 val) {
			return val.w != InvalidColorValueW;
		}

#ifndef SVGF_TemporalSupersamplingReverseReproject_RootConstants
#define SVGF_TemporalSupersamplingReverseReproject_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT
#endif

#ifndef SVGF_CalcDepthPartialDerivative_RootConstants
#define SVGF_CalcDepthPartialDerivative_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_CalcDepthPartialDerivative_RCSTRUCT
#endif
#else
		const DXGI_FORMAT ValueMapFormat_Contrast = DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT ValueMapFormat_Color = HDR_FORMAT;
		const DXGI_FORMAT ValueSquaredMeanMapFormat_Contrast = DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT ValueSquaredMeanMapFormat_Color = HDR_FORMAT;

		const DXGI_FORMAT TSPPSquaredMeanRayHitDistanceMapFormat = DXGI_FORMAT_R16G16B16A16_UINT;
		const DXGI_FORMAT DepthPartialDerivativeMapFormat = DXGI_FORMAT_R16G16_FLOAT;
		const DXGI_FORMAT LocalMeanVarianceMapFormat = DXGI_FORMAT_R32G32_FLOAT;
		const DXGI_FORMAT VarianceMapFormat = DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT RayHitDistanceFormat = DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT TSPPMapFormat = DXGI_FORMAT_R8_UINT;
		const DXGI_FORMAT DisocclusionBlurStrengthMapFormat = DXGI_FORMAT_R8_UNORM;
#endif


		namespace RootConstant {
			namespace TemporalSupersamplingReverseReproject {
				struct Struct SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT
					enum {
					E_TexDim_X = 0,
					E_TexDim_Y,
					E_InvTexDim_X,
					E_InvTexDim_Y,
					Count
				};
			}

			namespace CalcDepthPartialDerivative {
				struct Struct SVGF_CalcDepthPartialDerivative_RCSTRUCT
					enum {
					E_InvTexDim_X = 0,
					E_InvTexDim_Y,
					Count
				};
			}

			namespace AtrousWaveletTransformFilter {
				enum {
					E_RayHitDistToKernelWidthScale = 0,
					E_RayHitDistToKernelSizeScaleExp,
					Count
				};
			}

			namespace DisocclusionBlur {
				enum {
					E_TexDim_X = 0,
					E_TexDim_Y,
					E_Step,
					E_MaxStep,
					Count
				};
			}
		}
	}

	namespace RTAO {
#ifdef _HLSL
		typedef float	AOCoefficientMapFormat;
		typedef uint	TSPPMapFormat;
		typedef float	AOCoefficientSquaredMeanMapFormat;
		typedef float	RayHitDistanceFormat;

		static const float RayHitDistanceOnMiss = 0.f;
		static const float InvalidAOCoefficientValue = SVGF::InvalidContrastValue;

		bool HasAORayHitAnyGeometry(float tHit) {
			return tHit != RayHitDistanceOnMiss;
		}
#else
		const DXGI_FORMAT AOCoefficientMapFormat			= DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT TSPPMapFormat						= DXGI_FORMAT_R8_UINT;
		const DXGI_FORMAT AOCoefficientSquaredMeanMapFormat	= DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT RayHitDistanceFormat				= DXGI_FORMAT_R16_FLOAT;
#endif
	}

	namespace BlurFilter {
		namespace ThreadGroup {
			namespace Default {
				enum {
					Width = 8,
					Height = 8,
					Depth = 1,
					Size = Width * Height * Depth
				};
			}
		}

#ifndef BlurFilter_Default_RCSTRUCT
#define BlurFilter_Default_RCSTRUCT {	\
		DirectX::XMFLOAT2 gTexDim;		\
		DirectX::XMFLOAT2 gInvTexDim;	\
	};
#endif

#ifdef _HLSL
#ifndef BlurFilter_Default_RootConstants
#define BlurFilter_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) BlurFilter_Default_RCSTRUCT
#endif
#else
		namespace RootConstant {
			namespace Default {
				struct Struct BlurFilter_Default_RCSTRUCT
				enum {
					E_TexDimX = 0,
					E_TexDimY,
					E_InvTexDimX,
					E_InvTexDimY,
					Count
				};
			}
		}
#endif
	}
}

#endif // __SHADINGCONVENTION_H__