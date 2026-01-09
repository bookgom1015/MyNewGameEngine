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
			bool Enabled = true;
			const float MaxGamma = 3.f;
			const float MinGamma = 0.5f;
			float Gamma = 2.2f;
		};

		struct ToneMappingArguments {
			const float MaxExposure = 8.f;
			const float MinExposure = 0.01f;
			float Exposure = 1.4f;

			const float MaxMiddleGrayKey = 0.99f;
			const float MinMiddleGrayKey = 0.01f;
			float MiddleGrayKey = 0.36f;

			std::uint32_t TonemapperType = 0;
		};

		struct TAAArguments {
			bool Enabled = true;
			float ModulationFactor = 0.9f;
		};

		struct BlendWithCurrentFrameArguments {
			bool UseClamping = true;
			float DepthSigma = 1.f;
			float StdDevGamma = 0.6f;
			float MinStdDevTolerance = 0.05f;

			float ClampDifferenceToTSPPScale = 4.f;
			std::uint32_t MinTSPPToUseTemporalVariance = 4;
			std::uint32_t LowTSPPMaxTSPP = 12;
			float LowTSPPDecayConstant = 1.f;
		};

		struct AtrousWaveletTransformFilterArguments {
			float ValueSigma = 1.f;
			float NormalSigma = 64.f;
			float DepthSigma = 1.f;
			float DepthWeightCutoff = 0.2f;

			float MinVarianceToDenoise = 0.f;

			bool UseSmoothedVariance = false;

			bool PerspectiveCorrectDepthInterpolation = true;

			bool UseAdaptiveKernelSize = true;
			bool KernelRadiusRotateKernelEnabled = true;
			std::int32_t KernelRadiusRotateKernelNumCycles = 3;
			std::int32_t FilterMinKernelWidth = 3;
			float FilterMaxKernelWidthPercentage = 1.5f;
			float AdaptiveKernelSizeRayHitDistanceScaleFactor = 0.02f;
			float AdaptiveKernelSizeRayHitDistanceScaleExponent = 2.f;
		};

		struct DenoiserArguments {
			bool DisocclusionBlurEnabled = true;
			bool FullscreenBlurEnabaled = true;
			bool UseSmoothingVariance = true;
			std::uint32_t LowTsppBlurPassCount = 3;
		};

		struct SSAOArguments {
			const float MinOcclusionRadius = 0.1f;
			const float MaxOcclusionRadius = 10.f;
			float OcclusionRadius = 4.f;

			const float MinOcclusionFadeStart = 0.f;
			float OcclusionFadeStart = 1.f;

			const float MaxOcclusionFadeEnd = 10.f;
			float OcclusionFadeEnd = 8.f;

			const float MinOcclusionStrength = 1.f;
			const float MaxOcclusionStrength = 10.f;
			float OcclusionStrength = 2.f;

			const float MinSurfaceEpsilon = 0.001f;
			const float MaxSurfaceEpsilon = 0.1f;
			float SurfaceEpsilon = 0.05f;

			const std::uint32_t MaxSampleCount = 14;
			const std::uint32_t MinSampleCount = 1;
			std::uint32_t SampleCount = 6;

			BlendWithCurrentFrameArguments BlendWithCurrentFrame;
			AtrousWaveletTransformFilterArguments AtrousWaveletTransformFilter;
			DenoiserArguments Denoiser;
		};

		struct RTAOArguments {
			float OcclusionRadius = 10.f;
			float OcclusionFadeStart = 0.f;
			float OcclusionFadeEnd = 10.f;
			float SurfaceEpsilon = 0.0001f;

			std::uint32_t SampleCount = 1;
			const std::uint32_t MaxSampleCount = 128;
			const std::uint32_t MinSampleCount = 1;

			std::uint32_t SampleSetSize = 8;
			const std::uint32_t MaxSampleSetSize = 8;
			const std::uint32_t MinSampleSetSize = 1;

			// RaySorting
			bool RaySortingEnabled = true;
			bool CheckerboardGenerateRaysForEvenPixels = false;
			bool CheckboardRayGeneration = true;
			bool RandomFrameSeed = true;

			BlendWithCurrentFrameArguments BlendWithCurrentFrame;
			AtrousWaveletTransformFilterArguments AtrousWaveletTransformFilter;
			DenoiserArguments Denoiser;

			//
			std::uint32_t MaxTSPP = 33;
		};

		struct RaySortingArguments {
			float DepthBinSizeMultiplier = 0.1f;
			const float MinDepthBinSizeMultiplier = 0.01f;
			const float MaxDepthBinSizeMultiplier = 10.f;
		};

		struct VolumetricLightArguments {
			bool Enabled = true;
			float DepthExponent = 4.f;

			const float MinAnisotropicCoefficient = -0.5f;
			const float MaxAnisotropicCoefficient = 0.5f;
			float AnisotropicCoefficient = 0.f;

			const float MaxUniformDensity = 1.f;
			const float MinUniformDensity = 0.f;
			float UniformDensity = 0.1f;

			const float MaxDensityScale = 1.f;
			const float MinDensityScale = 0.f;
			float DensityScale = 0.01f;

			bool TricubicSamplingEnabled = true;
		};

		struct SSCSArguments {
			bool Enabled = true;

			const std::uint32_t MaxStep = 32;
			const std::uint32_t MinStep = 8;
			std::uint32_t Steps = 8;

			float RayMaxDistance = 0.3f;

			const float MaxThcikness = 0.015f;
			const float MinThcikness = 0.008f;
			float Thcikness = 0.01f;

			const float MaxBiasBase = 0.01f;
			const float MinBiasBase = 0.002f;
			float BiasBase = 0.003f;

			const float MaxBiasSlope = 0.1f;
			const float MinBiasSlope = 0.01f;
			float BiasSlope = 0.022f;

			const float MaxDepthEpsilonBase = 0.01f;
			const float MinDepthEpsilonBase = 0.001f;
			float DepthEpsilonBase = 0.002f;

			const float MaxDepthEpsilonScale = 0.02f;
			const float MinDepthEpsilonScale = 0.001f;
			float DepthEpsilonScale = 0.009f;

			const float MaxStepScaleFar = 2.5f;
			const float MinStepScaleFar = 1.5f;
			float StepScaleFar = 2.f;

			const float MaxStepScaleFarDist = 60.f;
			const float MinStepScaleFarDist = 20.f;
			float StepScaleFarDist = 40.f;

			const float MaxThicknessFarScale = 1.5f;
			const float MinThicknessFarScale = 3.f;
			float ThicknessFarScale = 2.f;

			const float MaxThicknessFarDist = 60.f;
			const float MinThicknessFarDist = 20.f;
			float ThicknessFarDist = 40.f;
		};

		struct MotionBlurArguments {
			bool Enabled = true;

			float Intensity = 0.01f;
			float Limit = 0.005f;
			float DepthBias = 0.05f;

			const std::int32_t MaxSampleCount = 32;
			const std::int32_t MinSampleCount = 8;
			std::int32_t SampleCount = 16;
		};

		struct BloomArguments {
			bool Enabled = true;

			float Threshold = 1.f;
			float SoftKnee = 0.5f;
		};

		struct DOFArguments {
			bool Enabled = false;

			const std::uint32_t MaxBokehSampleCount = 10;
			const std::uint32_t MinBokehSampleCount = 1;
			std::uint32_t BokehSampleCount = 4;

			const float MaxBokehRadius = 10.f;
			const float MinBokehRadius = 1.f;
			float BokehRadius = 2.f;

			const float MaxBokehThreshold = 0.9f;
			const float MinBokehThreshold = 0.1f;
			float BokehThreshold = 0.9f;

			const float MaxHighlightPower = 8.f;
			const float MinHighlightPower = 1.f;
			float HighlightPower = 2.f;
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

			bool ShadowEnabled = true;
			bool AOEnabled = true;
			bool RaytracingEnabled = false;
		};
	}
}