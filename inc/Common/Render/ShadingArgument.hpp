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
			FLOAT OcclusionRadius = 0.5f;
			FLOAT OcclusionFadeStart = 0.2f;
			FLOAT OcclusionFadeEnd = 2.f;
			FLOAT OcclusionStrength = 4.f;
			FLOAT SurfaceEpsilon = 0.05f;
			UINT SampleCount = 14;
			UINT BlurCount = 3;
		};

		struct RTAOArguments {
			FLOAT OcclusionRadius = 10.f;
			FLOAT OcclusionFadeStart = 0.001f;
			FLOAT OcclusionFadeEnd = 10.f;
			FLOAT OcclusionStrength = 4.f;
			FLOAT SurfaceEpsilon = 0.001f;
			UINT SampleCount = 2;
		};

		struct ShadingArgumentSet {
			GammaCorrectionArguments GammaCorrection;			
			ToneMappingArguments ToneMapping;
			TAAArguments TAA;
			SSAOArguments SSAO;
			RTAOArguments RTAO;

			BOOL ShadowEnabled = TRUE;
			BOOL AOEnabled = TRUE;
			BOOL RaytracingEnabled = FALSE;
		};
	}
}