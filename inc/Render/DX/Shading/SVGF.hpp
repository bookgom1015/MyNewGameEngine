#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {	
	namespace SVGF {
		namespace Shader {
			enum Type {
				CS_TemporalSupersamplingReverseReproject_Contrast = 0,
				CS_TemporalSupersamplingReverseReproject_Color,
				CS_TemporalSupersamplingBlendWithCurrentFrame_Contrast,
				CS_TemporalSupersamplingBlendWithCurrentFrame_Color,
				CS_CalcParticalDepthDerivative,
				CS_CalcLocalMeanVariance_Contrast,
				CS_CalcLocalMeanVariance_Color,
				CS_FillinCheckerboard,
				CS_EdgeStoppingFilterGaussian3x3_Contrast,
				CS_EdgeStoppingFilterGaussian3x3_Color,
				CS_DisocclusionBlur3x3_Contrast,
				CS_DisocclusionBlur3x3_Color,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				E_TemporalSupersamplingReverseReproject = 0,
				E_TemporalSupersamplingBlendWithCurrentFrame,
				E_CalcDepthPartialDerivative,
				E_CalcLocalMeanVariance,
				E_FillInCheckerboard,
				E_AtrousWaveletTransformFilter,
				E_DisocclusionBlur,
				Count
			};

			namespace TemporalSupersamplingReverseReproject {
				enum {
					CB_CrossBilateralFilter = 0,
					RC_Consts,
					SI_NormalDepth,
					SI_ReprojectedNormalDepth,
					SI_Velocity,
					SI_DepthPartialDerivative,
					SI_CachedNormalDepth,
					SI_CachedValue,
					SI_CachedValueSquaredMean,
					SI_CachedTSPP,
					SI_CachedRayHitDistance,
					UO_CachedTSPP,
					UO_CachedValue,
					UO_CachedSquaredMean,
					UO_TSPPSquaredMeanRayHitDistacne,
					UO_DebugMap0,
					UO_DebugMap1,
					Count
				};
			}

			namespace TemporalSupersamplingBlendWithCurrentFrame {
				enum {
					CB_TSPPBlendWithCurrentFrame = 0,
					SI_AOCoefficient,
					SI_LocalMeanVaraince,
					SI_RayHitDistance,
					SI_CachedValue,
					SI_CachedSquaredMean,
					SI_TSPPSquaredMeanRayHitDistance,
					UO_TemporalAOCoefficient,
					UO_TSPP,
					UO_AOCoefficientSquaredMean,
					UO_RayHitDistance,
					UO_VarianceMap,
					UO_BlurStrength,
					Count
				};
			}

			namespace CalcDepthPartialDerivative {
				enum {
					RC_Consts = 0,
					SI_DepthMap,
					UO_DepthPartialDerivative,
					Count
				};				
			}

			namespace CalcLocalMeanVariance {
				enum {
					CB_LocalMeanVariance = 0,
					SI_AOCoefficient,
					UO_LocalMeanVariance,
					Count
				};
			}

			namespace FillInCheckerboard {
				enum {
					CB_LocalMeanVariance = 0,
					UIO_LocalMeanVariance,
					Count
				};
			}

			namespace AtrousWaveletTransformFilter {
				enum {
					CB_AtrousFilter = 0,
					RC_Consts,
					SI_TemporalValue,
					SI_NormalDepth,
					SI_Variance,
					SI_HitDistance,
					SI_DepthPartialDerivative,
					SI_TSPP,
					UO_TemporalValue,
					Count
				};
			}

			namespace DisocclusionBlur {
				enum {
					RC_Consts = 0,
					SI_DepthMap,
					SI_BlurStrength,
					SI_RoughnessMetalnessMap,
					UIO_AOCoefficient,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				E_TemporalSupersamplingReverseReproject_Contrast = 0,
				E_TemporalSupersamplingReverseReproject_Color,
				E_TemporalSupersamplingBlendWithCurrentFrame_Contrast,
				E_TemporalSupersamplingBlendWithCurrentFrame_Color,
				E_CalcDepthPartialDerivative,
				E_CalcLocalMeanVariance_Contrast,
				E_CalcLocalMeanVariance_Color,
				E_FillInCheckerboard,
				E_EdgeStoppingFilterGaussian3x3_Contrast,
				E_EdgeStoppingFilterGaussian3x3_Color,
				E_DisocclusionBlur_Contrast,
				E_DisocclusionBlur_Color,
				Count
			};
		}

		namespace Resource {
			namespace LocalMeanVariance {
				enum {
					E_Raw = 0,
					E_Smoothed,
					Count
				};
			}

			namespace Variance {
				enum {
					E_Raw = LocalMeanVariance::Count,
					E_Smoothed,
					Count
				};
			}

			namespace CachedValue {
				enum {
					E_Contrast = Variance::Count,
					E_Color,
					Count
				};
			}

			namespace CachedSquaredMean {
				enum {
					E_Contrast = CachedValue::Count,
					E_Color,
					Count
				};
			}

			enum {
				E_DepthPartialDerivative = CachedSquaredMean::Count,
				E_DisocclusionBlurStrength,
				E_TSPPSquaredMeanRayHitDistance,
				Count
			};
		}

		namespace Descriptor {
			namespace LocalMeanVariance {
				enum {
					ES_Raw = 0,
					EU_Raw,
					ES_Smoothed,
					EU_Smoothed,
					Count
				};
			}

			namespace Variance {
				enum {
					ES_Raw = LocalMeanVariance::Count,
					EU_Raw,
					ES_Smoothed,
					EU_Smoothed,
					Count
				};
			}

			namespace CachedValue {
				enum {
					ES_Contrast = Variance::Count,
					EU_Contrast,
					ES_Color,
					EU_Color,
					Count
				};
			}

			namespace CachedSquaredMean {
				enum {
					ES_Contrast = CachedValue::Count,
					EU_Contrast,
					ES_Color,
					EU_Color,
					Count
				};
			}

			enum {
				ES_DepthPartialDerivative = CachedSquaredMean::Count,
				EU_DepthPartialDerivative,
				ES_DisocclusionBlurStrength,
				EU_DisocclusionBlurStrength,
				ES_TSPPSquaredMeanRayHitDistance,
				EU_TSPPSquaredMeanRayHitDistance,
				Count
			};
		}

		namespace Value {
			enum Type {
				E_Contrast,
				E_Color
			};
		}

		class SVGFClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
				UINT ClientWidth = 0;
				UINT ClientHeight = 0;
			};

		public:
			SVGFClass();
			virtual ~SVGFClass() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) override;
			virtual BOOL BuildPipelineStates() override;
			virtual BOOL BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) override;
			virtual BOOL OnResize(UINT width, UINT height) override;

		public:
			BOOL CalculateDepthParticalDerivative(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap);
			BOOL CalculateLocalMeanVariance(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pValueMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_valueMap,
				Value::Type type,
				BOOL bCheckerboardSamplingEnabled);
			BOOL FillInCheckerboard(
				Foundation::Resource::FrameResource* const pFrameResource,
				BOOL bCheckerboardSamplingEnabled);

			BOOL ReverseReprojectPreviousFrame(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
				Foundation::Resource::GpuResource* const pReprojNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_reprojNormalDepthMap,
				Foundation::Resource::GpuResource* const pCachedNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_cachedNormalDepthMap,
				Foundation::Resource::GpuResource* const pVelocityMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap,
				Foundation::Resource::GpuResource* const pCachedValueMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_cachedValueMap,
				Foundation::Resource::GpuResource* const pCachedValueSquaredMeanMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_cachedValueSquaredMeanMap,
				Foundation::Resource::GpuResource* const pCachedRayHitDistMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_cachedRayHitDistMap,
				Foundation::Resource::GpuResource* const pCachedTSPPMap0,
				D3D12_GPU_DESCRIPTOR_HANDLE si_cachedTSPPMap,
				Foundation::Resource::GpuResource* const pCachedTSPPMap1,
				D3D12_GPU_DESCRIPTOR_HANDLE uo_cachedTSPPMap,
				Value::Type type);
			BOOL BlendWithCurrentFrame(
				Foundation::Resource::FrameResource* const pFrameResource, 
				Foundation::Resource::GpuResource* const pValueMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_valueMap,
				Foundation::Resource::GpuResource* const pRayHitDistanceMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_rayHitDistanceMap,
				Foundation::Resource::GpuResource* const pTemporalCacheValueMap, 
				D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheValueMap,
				Foundation::Resource::GpuResource* const pTemporalCacheValueSquaredMeanMap,
				D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheValueSquaredMeanMap,
				Foundation::Resource::GpuResource* const pTemporalCacheRayHitDistanceMap,
				D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheRayHitDistanceMap,
				Foundation::Resource::GpuResource* const pTemporalTSPPMap,
				D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheTSPPMap,
				Value::Type type);
			BOOL ApplyAtrousWaveletTransformFilter(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
				Foundation::Resource::GpuResource* const pTemporalCacheHitDistanceMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_temporalCachehitDistanceMap,
				Foundation::Resource::GpuResource* const pTemporalCacheTSPPMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_temporalCacheTSPPMap,
				Foundation::Resource::GpuResource* const pTemporalValueMap_Input,
				D3D12_GPU_DESCRIPTOR_HANDLE si_temporalValueMap, 
				Foundation::Resource::GpuResource* const pTemporalValueMap_Output,
				D3D12_GPU_DESCRIPTOR_HANDLE uo_TemporalValueMap,
				Value::Type type,
				FLOAT rayHitDistToKernelWidthScale,
				FLOAT rayHitDistToKernelSizeScaleExp);
			BOOL BlurDisocclusion(
				Foundation::Resource::FrameResource* const pFrameResource, 
				Foundation::Resource::GpuResource* const pDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
				Foundation::Resource::GpuResource* const pRoughnessMetalnessMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap,
				Foundation::Resource::GpuResource* const pTemporalValueMap,
				D3D12_GPU_DESCRIPTOR_HANDLE uio_temporalValueMap, 
				Value::Type type,
				UINT numLowTSPPBlurPasses);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array < Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::Count> mResources;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Descriptor::Count> mhCpuDecs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Descriptor::Count> mhGpuDecs;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, 2> mDebugMaps;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> mhDebugMapCpuUavs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2> mhDebugMapGpuUavs;
		};

		using InitDataPtr = std::unique_ptr<SVGFClass::InitData>;

		InitDataPtr MakeInitData();

		__forceinline UINT NumMantissaBitsInFloatFormat(UINT floatFormatBitLength);
	}
}

UINT Render::DX::Shading::SVGF::NumMantissaBitsInFloatFormat(UINT floatFormatBitLength) {
	switch (floatFormatBitLength) {
	case 32: return 23;
	case 16: return 10;
	case 11: return 6;
	case 10: return 5;
	}
	return 0;
}