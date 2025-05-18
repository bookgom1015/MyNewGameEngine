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
				CS_ParticalDepthDerivative,
				CS_CalcLocalMeanVariance,
				CS_FillinCheckerBoard,
				CS_EdgeStoppingFilterGaussian3x3,
				CS_DisocclusionBlur3x3,
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
					SI_CachedNormalDepth,
					SI_DepthPartialDerivative,
					SI_Velocity,
					SI_CachedAOCoefficient,
					SI_CachedTSPP,
					SI_CachedAOCoefficientSquaredMean,
					SI_CachedRayHitDistance,
					UO_CachedTSPP,
					UO_CachedValue,
					UO_CachedSquaredMean,
					UO_TSPPSquaredMeanRayHitDistacne,
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
					UIO_TemporalAOCoefficient,
					UIO_TSPP,
					UIO_AOCoefficientSquaredMean,
					UIO_RayHitDistance,
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
					SI_TemporalAOCoefficient,
					SI_NormalDepth,
					SI_Variance,
					SI_HitDistance,
					SI_DepthPartialDerivative,
					SI_TSPP,
					UO_TemporalAOCoefficient,
					Count
				};
			}

			namespace DisocclusionBlur {
				enum {
					RC_Consts = 0,
					SI_DepthMap,
					SI_BlurStrength,
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
				E_AtrousWaveletTransformFilter_Contrast,
				E_AtrousWaveletTransformFilter_Color,
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
		};

		using InitDataPtr = std::unique_ptr<SVGFClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

