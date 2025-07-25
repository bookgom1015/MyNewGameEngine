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
			FLOAT Gamma = 2.2f;
		};

		struct ToneMappingArguments {
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

			BOOL RaySortingEnabled = TRUE;
			BOOL CheckerboardGenerateRaysForEvenPixels = FALSE;
			BOOL CheckboardRayGeneration = TRUE;
			BOOL RandomFrameSeed = TRUE;
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