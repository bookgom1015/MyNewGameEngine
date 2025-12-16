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

			UINT TonemapperType = 0;
		};

		struct TAAArguments {
			BOOL Enabled = TRUE;
			FLOAT ModulationFactor = 0.9f;
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

		struct SSAOArguments {
			const FLOAT MinOcclusionRadius = 0.1f;
			const FLOAT MaxOcclusionRadius = 10.f;
			FLOAT OcclusionRadius = 4.f;

			const FLOAT MinOcclusionFadeStart = 0.f;
			FLOAT OcclusionFadeStart = 1.f;

			const FLOAT MaxOcclusionFadeEnd = 10.f;
			FLOAT OcclusionFadeEnd = 8.f;

			const FLOAT MinOcclusionStrength = 1.f;
			const FLOAT MaxOcclusionStrength = 10.f;
			FLOAT OcclusionStrength = 2.f;

			const FLOAT MinSurfaceEpsilon = 0.001f;
			const FLOAT MaxSurfaceEpsilon = 0.1f;
			FLOAT SurfaceEpsilon = 0.05f;

			const UINT MaxSampleCount = 14;
			const UINT MinSampleCount = 1;
			UINT SampleCount = 6;

			BlendWithCurrentFrameArguments BlendWithCurrentFrame;
			AtrousWaveletTransformFilterArguments AtrousWaveletTransformFilter;
			DenoiserArguments Denoiser;
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

		struct VolumetricLightArguments {
			BOOL Enabled = TRUE;
			FLOAT DepthExponent = 4.f;

			const FLOAT MinAnisotropicCoefficient = -0.5f;
			const FLOAT MaxAnisotropicCoefficient = 0.5f;
			FLOAT AnisotropicCoefficient = 0.f;

			const FLOAT MaxUniformDensity = 1.f;
			const FLOAT MinUniformDensity = 0.f;
			FLOAT UniformDensity = 0.1f;

			const FLOAT MaxDensityScale = 1.f;
			const FLOAT MinDensityScale = 0.f;
			FLOAT DensityScale = 0.01f;

			BOOL TricubicSamplingEnabled = TRUE;
		};

		struct SSCSArguments {
			BOOL Enabled = TRUE;

			UINT MaxSteps = 8;
			FLOAT RayMaxDistance = 0.3f;
			FLOAT Thcikness = 0.02f;
		};

		struct MotionBlurArguments {
			BOOL Enabled = TRUE;

			FLOAT Intensity = 0.01f;
			FLOAT Limit = 0.005f;
			FLOAT DepthBias = 0.05f;
			INT SampleCount = 16;
		};

		struct BloomArguments {
			BOOL Enabled = TRUE;

			FLOAT Threshold = 1.f;
			FLOAT SoftKnee = 0.5f;
		};

		struct DOFArguments {
			BOOL Enabled = FALSE;
		};

		struct ShadingArgumentSet {
			GammaCorrectionArguments GammaCorrection;			
			ToneMappingArguments ToneMapping;
			TAAArguments TAA;
			SSAOArguments SSAO;
			RTAOArguments RTAO;
			RaySortingArguments RaySorting;
			VolumetricLightArguments VolumetricLight;
			SSCSArguments SSCS;
			MotionBlurArguments MotionBlur;
			BloomArguments Bloom;
			DOFArguments DOF;

			BOOL ShadowEnabled = TRUE;
			BOOL AOEnabled = TRUE;
			BOOL RaytracingEnabled = FALSE;
		};
	}
}