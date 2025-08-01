#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

namespace Common::Render {
	namespace ShadingArgument {
		struct GammaCorrectionArguments {
			BOOL Enabled = TRUE;
			const FLOAT MaxGamma = 3.f;
			const FLOAT MinGamma = 0.5f;
			FLOAT Gamma = 2.2f;
		};

		struct ToneMappingArguments {
			const FLOAT MaxExposure = 8.f;
			const FLOAT MinExposure = 0.01f;
			FLOAT Exposure = 1.4f;
		};

		struct TAAArguments {
			BOOL Enabled = TRUE;
			FLOAT ModulationFactor = 0.9f;
		};

		struct SSAOArguments {
			FLOAT OcclusionRadius = 1.f;
			FLOAT OcclusionFadeStart = 0.f;
			FLOAT OcclusionFadeEnd = 1.f;
			FLOAT OcclusionStrength = 4.f;
			FLOAT SurfaceEpsilon = 0.05f;
			UINT SampleCount = 6;
			UINT BlurCount = 3;
		};

		struct BlendWithCurrentFrameArguments {
			BOOL UseClamping = TRUE;
			FLOAT DepthSigma = 1.f;
			FLOAT StdDevGamma = 0.6f;
			FLOAT MinStdDevTolerance = 0.05f;

			FLOAT ClampDifferenceToTSPPScale = 4.f;
			UINT MinTSPPToUseTemporalVariance = 4;
			UINT LowTSPPMaxTSPP = 12;
			FLOAT LowTSPPDecayConstant = 1.f;
		};

		struct AtrousWaveletTransformFilterArguments {
			FLOAT ValueSigma = 1.f;
			FLOAT NormalSigma = 64.f;
			FLOAT DepthSigma = 1.f;
			FLOAT DepthWeightCutoff = 0.2f;

			FLOAT MinVarianceToDenoise = 0.f;

			BOOL UseSmoothedVariance = FALSE;

			BOOL PerspectiveCorrectDepthInterpolation = TRUE;

			BOOL UseAdaptiveKernelSize = TRUE;
			BOOL KernelRadiusRotateKernelEnabled = TRUE;
			INT KernelRadiusRotateKernelNumCycles = 3;
			INT FilterMinKernelWidth = 3;
			FLOAT FilterMaxKernelWidthPercentage = 1.5f;
			FLOAT AdaptiveKernelSizeRayHitDistanceScaleFactor = 0.02f;
			FLOAT AdaptiveKernelSizeRayHitDistanceScaleExponent = 2.f;
		};

		struct DenoiserArguments {
			BOOL DisocclusionBlurEnabled = TRUE;
			BOOL FullscreenBlurEnabaled = TRUE;
			BOOL UseSmoothingVariance = TRUE;
			UINT LowTsppBlurPassCount = 3;
		};

		struct RTAOArguments {
			FLOAT OcclusionRadius = 10.f;
			FLOAT OcclusionFadeStart = 0.f;
			FLOAT OcclusionFadeEnd = 10.f;
			FLOAT SurfaceEpsilon = 0.0001f;

			UINT SampleCount = 1;
			const UINT MaxSampleCount = 128;
			const UINT MinSampleCount = 1;

			UINT SampleSetSize = 8;
			const UINT MaxSampleSetSize = 8;
			const UINT MinSampleSetSize = 1;

			// RaySorting
			BOOL RaySortingEnabled = TRUE;
			BOOL CheckerboardGenerateRaysForEvenPixels = FALSE;
			BOOL CheckboardRayGeneration = TRUE;
			BOOL RandomFrameSeed = TRUE;

			BlendWithCurrentFrameArguments BlendWithCurrentFrame;
			AtrousWaveletTransformFilterArguments AtrousWaveletTransformFilter;
			DenoiserArguments Denoiser;

			//
			UINT MaxTSPP = 33;
		};

		struct RaySortingArguments {
			FLOAT DepthBinSizeMultiplier = 0.1f;
			const FLOAT MinDepthBinSizeMultiplier = 0.01f;
			const FLOAT MaxDepthBinSizeMultiplier = 10.f;
		};

		struct ShadingArgumentSet {
			GammaCorrectionArguments GammaCorrection;			
			ToneMappingArguments ToneMapping;
			TAAArguments TAA;
			SSAOArguments SSAO;
			RTAOArguments RTAO;
			RaySortingArguments RaySorting;

			BOOL ShadowEnabled = TRUE;
			BOOL AOEnabled = TRUE;
			BOOL RaytracingEnabled = FALSE;
		};
	}
}