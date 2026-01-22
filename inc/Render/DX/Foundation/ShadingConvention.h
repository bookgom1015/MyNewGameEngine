#ifndef __SHADINGCONVENTION_H__
#define __SHADINGCONVENTION_H__

#ifdef _HLSL
	#ifndef HDR_FORMAT
	#define HDR_FORMAT float4
	#endif

	#ifndef SDR_FORMAT
	#define SDR_FORMAT float4
	#endif

	#ifndef AOMAP_FORMAT
	#define AOMAP_FORMAT FLOAT
	#endif
#else
	#ifndef HDR_FORMAT
	#define HDR_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
	#endif

	#ifndef SDR_FORMAT
	#define SDR_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM
	#endif

	#ifndef AOMAP_FORMAT
	#define AOMAP_FORMAT DXGI_FORMAT_R16_FLOAT
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
		typedef FLOAT DepthBufferFormat;
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
		FLOAT gMiddleGrayKey;				\
		UINT gTonemapperType;				\
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
				E_MiddleGray,
				E_TonemapperType,
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
		DirectX::XMUINT2 gTexDim;	\
		UINT gVertexCount;			\
		UINT gIndexCount;			\
		FLOAT gDitheringMaxDist;	\
		FLOAT gDitheringMinDist;	\
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
					E_TexDim_X = 0,
					E_TexDim_Y,
					E_VertexCount,
					E_IndexCount,
					E_MaxDitheringDist,
					E_MinDitheringDist,
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
					E_SSAOEnabled = 0,
					Count
				};
			}
		}
#endif
	}

	namespace Shadow {
		static const UINT CascadeCount = 3;

		namespace ShadowMap {
			enum Type {
				E_Texture2D = 0,
				E_Texture2DArray,
				E_CubeMap,
				Count
			};
		}

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
		typedef FLOAT	ZDepthMapFormat;
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
				Depth						= 1,
				Size						= Width * Height * Depth
			};
		}

#ifdef _HLSL
		typedef uint2 RayIndexOffsetMapFormat;
#else
		static_assert(
			RayGroup::Width <= 64 &&
			RayGroup::Height <= 128 &&
			RayGroup::Size <= 8192,
			"Ray group dimensions are outside the supported limits set by the Counting Sort shader.");

		const DXGI_FORMAT RayIndexOffsetMapFormat = DXGI_FORMAT_R8G8_UINT;
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

#ifndef SVGF_AtrousWaveletTransformFilter_RCSTRUCT
#define SVGF_AtrousWaveletTransformFilter_RCSTRUCT {	\
		FLOAT gRayHitDistanceToKernelWidthScale;		\
		FLOAT gRayHitDistanceToKernelSizeScaleExponent;	\
	};
#endif

#ifndef SVGF_DisocclusionBlur_RCSTRUCT
#define SVGF_DisocclusionBlur_RCSTRUCT {	\
		DirectX::XMUINT2	gTextureDim;	\
		UINT				gStep;			\
		UINT				gMaxStep;		\
	};
#endif

#ifdef _HLSL
		typedef FLOAT		ValueMapFormat;
		typedef FLOAT		ValueSquaredMeanMapFormat;

		typedef uint4		TSPPSquaredMeanRayHitDistanceMapFormat;
		typedef float2		DepthPartialDerivativeMapFormat;
		typedef float2		LocalMeanVarianceMapFormat;
		typedef FLOAT		VarianceMapFormat;
		typedef FLOAT		RayHitDistanceMapFormat;
		typedef uint		TSPPMapFormat;
		typedef FLOAT		DisocclusionBlurStrengthMapFormat;

		typedef float4 DebugMapFormat;

		static const FLOAT InvalidValue = -1.f;

#ifndef SVGF_TemporalSupersamplingReverseReproject_RootConstants
#define SVGF_TemporalSupersamplingReverseReproject_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT
#endif

#ifndef SVGF_CalcDepthPartialDerivative_RootConstants
#define SVGF_CalcDepthPartialDerivative_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_CalcDepthPartialDerivative_RCSTRUCT
#endif

#ifndef SVGF_AtrousWaveletTransformFilter_RootConstants
#define SVGF_AtrousWaveletTransformFilter_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_AtrousWaveletTransformFilter_RCSTRUCT
#endif

#ifndef SVGF_DisocclusionBlur_RootConstants
#define SVGF_DisocclusionBlur_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_DisocclusionBlur_RCSTRUCT
#endif
#else
		const DXGI_FORMAT ValueMapFormat = DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT ValueSquaredMeanMapFormat = DXGI_FORMAT_R16_FLOAT;

		const DXGI_FORMAT TSPPSquaredMeanRayHitDistanceMapFormat = DXGI_FORMAT_R16G16B16A16_UINT;
		const DXGI_FORMAT DepthPartialDerivativeMapFormat = DXGI_FORMAT_R16G16_FLOAT;
		const DXGI_FORMAT LocalMeanVarianceMapFormat = DXGI_FORMAT_R32G32_FLOAT;
		const DXGI_FORMAT VarianceMapFormat = DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT RayHitDistanceMapFormat = DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT TSPPMapFormat = DXGI_FORMAT_R8_UINT;
		const DXGI_FORMAT DisocclusionBlurStrengthMapFormat = DXGI_FORMAT_R8_UNORM;

		const DXGI_FORMAT DebugMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
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
				struct Struct SVGF_AtrousWaveletTransformFilter_RCSTRUCT
				enum {
					E_RayHitDistToKernelWidthScale = 0,
					E_RayHitDistToKernelSizeScaleExp,
					Count
				};
			}

			namespace DisocclusionBlur {
				struct Struct SVGF_DisocclusionBlur_RCSTRUCT
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
		typedef AOMAP_FORMAT	AOCoefficientMapFormat;
		typedef FLOAT			AOCoefficientSquaredMeanMapFormat;
		typedef float3			RandomVectorMapFormat;
		typedef float4			DebugMapFormat;

		static const float InvalidAOValue = -1.f;

#ifndef SSAO_Default_RootConstants
#define SSAO_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) SSAO_Default_RCSTRUCT
#endif
#else
		const DXGI_FORMAT AOCoefficientMapFormat = AOMAP_FORMAT;
		const DXGI_FORMAT AOCoefficientSquaredMeanMapFormat = DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT RandomVectorMapFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
		const DXGI_FORMAT DebugMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

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

	namespace RTAO {
#ifdef _HLSL
		typedef AOMAP_FORMAT	AOCoefficientMapFormat;
		typedef uint			TSPPMapFormat;
		typedef FLOAT			AOCoefficientSquaredMeanMapFormat;
		typedef FLOAT			RayHitDistanceMapFormat;

		static const FLOAT RayHitDistanceOnMiss = 0.f;
		static const FLOAT InvalidAOCoefficientValue = SVGF::InvalidValue;

		bool HasAORayHitAnyGeometry(FLOAT tHit) {
			return tHit != RayHitDistanceOnMiss;
		}
#else
		const DXGI_FORMAT AOCoefficientMapFormat			= AOMAP_FORMAT;
		const DXGI_FORMAT TSPPMapFormat						= DXGI_FORMAT_R8_UINT;
		const DXGI_FORMAT AOCoefficientSquaredMeanMapFormat	= DXGI_FORMAT_R16_FLOAT;
		const DXGI_FORMAT RayHitDistanceMapFormat			= DXGI_FORMAT_R16_FLOAT;
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

	namespace VolumetricLight {
		namespace ThreadGroup {
			namespace CalculateScatteringAndDensity {
				enum {
					Width	= 8,
					Height	= 8,
					Depth	= 8,
					Size	= Width * Height * Depth
				};
			}

			namespace AccumulateScattering {
				enum {
					Width	= 8,
					Height	= 8,
					Depth	= 1,
					Size	= Width * Height * Depth
				};
			}

			namespace BlendScattering {
				enum {
					Width = 8,
					Height = 8,
					Depth = 8,
					Size = Width * Height * Depth
				};
			}
		}

#ifndef VolumetricLight_CalculateScatteringAndDensity_RCSTRUCT
#define VolumetricLight_CalculateScatteringAndDensity_RCSTRUCT {\
		FLOAT gNearZ;											\
		FLOAT gFarZ;											\
		FLOAT gDepthExponent;									\
		FLOAT gUniformDensity;									\
		FLOAT gAnisotropicCoefficient;							\
		UINT  gFrameCount;										\
	};
#endif

#ifndef VolumetricLight_AccumulateScattering_RCSTRUCT 
#define VolumetricLight_AccumulateScattering_RCSTRUCT {	\
		FLOAT gNearZ;									\
		FLOAT gFarZ;									\
		FLOAT gDepthExponent;							\
		FLOAT gDensityScale;							\
	};
#endif

#ifndef VolumetricLight_ApplyFog_RCSTRUCT 
#define VolumetricLight_ApplyFog_RCSTRUCT {	\
		FLOAT gNearZ;						\
		FLOAT gFarZ;						\
		FLOAT gDepthExponent;				\
	};
#endif

#ifdef _HLSL
		typedef float4 FrustumVolumeMapFormat;

#ifndef VolumetricLight_CalculateScatteringAndDensity_RootConstants
#define VolumetricLight_CalculateScatteringAndDensity_RootConstants(reg) cbuffer cbRootConstants : register (reg) VolumetricLight_CalculateScatteringAndDensity_RCSTRUCT
#endif

#ifndef VolumetricLight_AccumulateScattering_RootConstants
#define VolumetricLight_AccumulateScattering_RootConstants(reg) cbuffer cbRootConstants : register (reg) VolumetricLight_AccumulateScattering_RCSTRUCT
#endif

#ifndef VolumetricLight_ApplyFog_RootConstants
#define VolumetricLight_ApplyFog_RootConstants(reg) cbuffer cbRootConstants : register (reg) VolumetricLight_ApplyFog_RCSTRUCT
#endif
#else
		const DXGI_FORMAT FrustumVolumeMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
#endif

		namespace RootConstant {
			namespace CalculateScatteringAndDensity {
				struct Struct VolumetricLight_CalculateScatteringAndDensity_RCSTRUCT
				enum {
					E_NearPlane = 0,
					E_FarPlane,
					E_DepthExponent,
					E_UniformDensity,
					E_AnisotropicCoefficient,
					E_FrameCount,
					Count
				};
			}

			namespace AccumulateScattering {
				struct Struct VolumetricLight_AccumulateScattering_RCSTRUCT
				enum {
					E_NearPlane = 0,
					E_FarPlane,
					E_DepthExponent,
					E_DensityScale,
					Count
				};
			}

			namespace ApplyFog {
				struct Struct VolumetricLight_ApplyFog_RCSTRUCT
					enum {
					E_NearPlane = 0,
					E_FarPlane,
					E_DepthExponent,
					Count
				};
			}
		}
	}

	namespace SSCS {
		namespace ThreadGroup {
			namespace ComputeContactShadow {
				enum {
					Width	= 8,
					Height	= 8,
					Depth	= 1,
					Size	= Width * Height * Depth
				};
			}

			namespace ApplyContactShadow {
				enum {
					Width = 8,
					Height = 8,
					Depth = 1,
					Size = Width * Height * Depth
				};
			}
		}

#ifdef _HLSL
		typedef uint ContactShadowMapFormat;
		typedef float4 DebugMapFormat;
#else
		const DXGI_FORMAT ContactShadowMapFormat = DXGI_FORMAT_R16_UINT;
		const DXGI_FORMAT DebugMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
#endif
	}

	namespace MotionBlur {
#ifndef MotionBlur_Default_RCSTRUCT
#define MotionBlur_Default_RCSTRUCT {	\
		FLOAT gIntensity;				\
		FLOAT gLimit;					\
		FLOAT gDepthBias;				\
		UINT  gSampleCount;				\
	};
#endif

#ifdef _HLSL
	#ifndef MotionBlur_Default_RootConstants
	#define MotionBlur_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) MotionBlur_Default_RCSTRUCT
	#endif
#else
		namespace RootConstant {
			namespace Default {
				struct Struct MotionBlur_Default_RCSTRUCT
				enum {
					E_Intensity = 0,
					E_Limit,
					E_DepthBias,
					E_SampleCount,
					Count
				};
			}
		}
#endif
	}

	namespace Bloom {
#ifndef Bloom_ExtractHighlights_RCSTRUCT
#define Bloom_ExtractHighlights_RCSTRUCT {	\
		FLOAT gThreshold;					\
		FLOAT gSoftKnee;					\
	};
#endif

#ifndef Bloom_BlendBloomWithDownSampled_RCSTRUCT
#define Bloom_BlendBloomWithDownSampled_RCSTRUCT {	\
		DirectX::XMFLOAT2 gInvTexDim;				\
	};
#endif

		namespace ThreadGroup {
			namespace Default {
				enum {
					Width	= 8,
					Height	= 8,
					Depth	= 1,
					Size	= Width * Height * Depth
				};
			}
		}

#ifdef _HLSL
	#ifndef Bloom_ExtractHighlights_RootConstants
	#define Bloom_ExtractHighlights_RootConstants(reg) cbuffer cbRootConstants : register(reg) Bloom_ExtractHighlights_RCSTRUCT
	#endif

	#ifndef Bloom_BlendBloomWithDownSampled_RootConstants
	#define Bloom_BlendBloomWithDownSampled_RootConstants(reg) cbuffer cbRootConstants : register(reg) Bloom_BlendBloomWithDownSampled_RCSTRUCT
	#endif

		typedef HDR_FORMAT HighlightMapFormat;
#else
		const DXGI_FORMAT HighlightMapFormat = HDR_FORMAT;
#endif

		namespace RootConstant {
			namespace ExtractHighlights {
				struct Struct Bloom_ExtractHighlights_RCSTRUCT
				enum {
					E_Threshold = 0,
					E_SoftKnee,
					Count
				};
			}

			namespace BlendBloomWithDownSampled {
				struct Struct Bloom_BlendBloomWithDownSampled_RCSTRUCT
					enum {
					E_InvTexDimX = 0,
					E_InvTexDimY,
					Count
				};
			}
		}
	}

	namespace TextureScaler {
#ifndef TextureScaler_DownSample6x6_RCSTRUCT
#define TextureScaler_DownSample6x6_RCSTRUCT {	\
		DirectX::XMUINT2 gSrcTexDim;			\
		DirectX::XMUINT2 gDstTexDim;			\
	};
#endif

		namespace ThreadGroup {
			namespace DownSample6x6 {
				enum {
					Width = 8,
					Height = 8,
					Depth = 1,
					Size = Width * Height * Depth
				};
			}
		}

#ifdef _HLSL
	#ifndef TextureScaler_DownSample6x6_RootConstants
	#define TextureScaler_DownSample6x6_RootConstants(reg) cbuffer cbRootConstants : register(reg) TextureScaler_DownSample6x6_RCSTRUCT
	#endif
#else		
#endif

		namespace RootConstant {
			namespace DownSample6x6 {
				struct Struct TextureScaler_DownSample6x6_RCSTRUCT
				enum {
					E_SrcTexDim_X = 0,
					E_SrcTexDim_Y,
					E_DstTexDim_X,
					E_DstTexDim_Y,
					Count
				};
			}
		}
	}

	namespace DOF {
#ifndef DOF_CircleOfConfusion_RCSTRUCT
#define DOF_CircleOfConfusion_RCSTRUCT {	\
		FLOAT gFocusRange;					\
	};
#endif

#ifndef DOF_Bokeh_RCSTRUCT
#define DOF_Bokeh_RCSTRUCT {			\
		DirectX::XMFLOAT2 gInvTexDim;	\
		INT gSampleCount;				\
		FLOAT gBokehRadius;				\
		FLOAT gThreshold;				\
		FLOAT gHighlightPower;			\
	};
#endif

#ifndef DOF_BokehBlur3x3_RCSTRUCT
#define DOF_BokehBlur3x3_RCSTRUCT {		\
		DirectX::XMUINT2 gTexDim;		\
		DirectX::XMFLOAT2 gInvTexDim;	\
	};
#endif

		namespace ThreadGroup {
			namespace Default {
				enum {
					Width	= 8,
					Height	= 8,
					Depth	= 1,
					Size	= Width * Height * Depth
				};
			}
		}

#ifdef _HLSL
	#ifndef DOF_CircleOfConfusion_RootConstants
	#define DOF_CircleOfConfusion_RootConstants(reg) cbuffer cbRootConstants : register(reg) DOF_CircleOfConfusion_RCSTRUCT
	#endif

	#ifndef DOF_Bokeh_RootConstants
	#define DOF_Bokeh_RootConstants(reg) cbuffer cbRootConstants : register(reg) DOF_Bokeh_RCSTRUCT
	#endif

	#ifndef DOF_BokehBlur3x3_RootConstants
	#define DOF_BokehBlur3x3_RootConstants(reg) cbuffer cbRootConstants : register(reg) DOF_BokehBlur3x3_RCSTRUCT
	#endif

		typedef float CircleOfConfusionMapFormat;
#else		
		const DXGI_FORMAT CircleOfConfusionMapFormat = DXGI_FORMAT_R16_SNORM;
#endif

		namespace RootConstant {
			namespace CircleOfConfusion {
				struct Struct DOF_CircleOfConfusion_RCSTRUCT
				enum {
					E_FocusRange = 0,
					Count
				};
			}

			namespace Bokeh {
				struct Struct DOF_Bokeh_RCSTRUCT
				enum {
					E_InvTexDim_X = 0,
					E_InvTexDim_Y,
					E_SampleCount,
					E_BokehRadius,
					E_Threshold,
					E_HighlightPower,
					Count
				};
			}

			namespace BokehBlur3x3 {
				struct Struct DOF_BokehBlur3x3_RCSTRUCT
					enum {
					E_TexDim_X = 0,
					E_TexDim_Y,
					E_InvTexDim_X,
					E_InvTexDim_Y,
					Count
				};
			}
		}
	}

	namespace EyeAdaption {
#define MAX_BIN_COUNT 128

#ifndef EyeAdaption_LuminanceHistogram_RCSTRUCT
#define EyeAdaption_LuminanceHistogram_RCSTRUCT {	\
		DirectX::XMUINT2 gTexDim;					\
		FLOAT gMinLogLum;							\
		FLOAT gMaxLogLum;							\
		UINT gBinCount;								\
	};
#endif

#ifndef EyeAdaption_PercentileExtract_RCSTRUCT
#define EyeAdaption_PercentileExtract_RCSTRUCT {	\
		FLOAT gMinLogLum;							\
		FLOAT gMaxLogLum;							\
		FLOAT gLowPercent;							\
		FLOAT gHighPercent;							\
		UINT gBinCount;								\
	};
#endif

#ifndef EyeAdaption_TemporalSmoothing_RCSTRUCT
#define EyeAdaption_TemporalSmoothing_RCSTRUCT {	\
		FLOAT gUpSpeed;								\
		FLOAT gGlareUpSpeed;						\
		FLOAT gDownSpeed;							\
		FLOAT gDeltaTime;							\
	};
#endif

		struct HistogramBin {
			UINT Count;
		};

		struct Result {
			FLOAT AvgLogLum;
			FLOAT LowLogLum;
			FLOAT HighLogLum;
			UINT LowBin;
			UINT HighBin;
			UINT TotalCount;
		};

		namespace ThreadGroup {
			namespace Default {
				enum {
					Width	= 8,
					Height	= 8,
					Depth	= 1,
					Size	= Width * Height * Depth
				};
			}
		}

#ifdef _HLSL
	#ifndef EyeAdaption_LuminanceHistogram_RootConstants
	#define EyeAdaption_LuminanceHistogram_RootConstants(reg) cbuffer cbRootConstants : register(reg) EyeAdaption_LuminanceHistogram_RCSTRUCT
	#endif

	#ifndef EyeAdaption_PercentileExtract_RootConstants
	#define EyeAdaption_PercentileExtract_RootConstants(reg) cbuffer cbRootConstants : register(reg) EyeAdaption_PercentileExtract_RCSTRUCT
	#endif

	#ifndef EyeAdaption_TemporalSmoothing_RootConstants
	#define EyeAdaption_TemporalSmoothing_RootConstants(reg) cbuffer cbRootConstants : register(reg) EyeAdaption_TemporalSmoothing_RCSTRUCT
	#endif
#else		
#endif

		namespace RootConstant {
			namespace LuminanceHistogram {
				struct Struct EyeAdaption_LuminanceHistogram_RCSTRUCT
				enum {
					E_TexDim_X = 0,
					E_TexDim_Y,
					E_MinLogLum,
					E_MaxLogLum,
					E_BinCount,
					Count
				};
			}

			namespace PercentileExtract {
				struct Struct EyeAdaption_PercentileExtract_RCSTRUCT
				enum {
					E_MinLogLum,
					E_MaxLogLum,
					E_LowPercent,
					E_HighPercent,
					E_BinCount,
					Count
				};
			}

			namespace TemporalSmoothing {
				struct Struct EyeAdaption_TemporalSmoothing_RCSTRUCT
				enum {
					E_UpSpeed,
					E_GlareUpSpeed,
					E_DownSpeed,
					E_DeltaTime,
					Count
				};
			}
		}
	}

	namespace ChromaticAberration {
#ifndef ChromaticAberration_Default_RCSTRUCT
#define ChromaticAberration_Default_RCSTRUCT {									  \
		DirectX::XMFLOAT2 gInvTexDim;											  \
		FLOAT  gStrength;	/* overall chroma strength (e.g. 1.0 ~ 5.0)			*/\
		FLOAT  gThreshold;	/* 0~1 : effect starts at this normalized radius	*/\
		FLOAT  gFeather;	/* 0~1 : smooth transition width (e.g. 0.1)			*/\
		UINT  gMaxShiftPx;	/* max shift in pixels at extreme edge (e.g. 2~8)	*/\
		FLOAT  gExponent;	/* curve control (e.g. 1~3)							*/\
	};
#endif

#ifdef _HLSL
	#ifndef ChromaticAberration_Default_RootConstants
	#define ChromaticAberration_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) ChromaticAberration_Default_RCSTRUCT
	#endif
#else
		namespace RootConstant {
			namespace Default {
				struct Struct ChromaticAberration_Default_RCSTRUCT
					enum {
					E_InvTexDim_X = 0,
					E_InvTexDim_Y,
					E_Strength,
					E_Threshold,
					E_Feather,
					E_MaxShiftPx,
					E_Exponent,
					Count
				};
			}
		}
#endif
	}
}

#endif // __SHADINGCONVENTION_H__