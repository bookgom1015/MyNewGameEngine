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
			FLOAT ModulationFactor = 0.9f;
		};

		struct ShadingArgumentSet {
			GammaCorrectionArguments GammaCorrection;			
			ToneMappingArguments ToneMapping;
			TAAArguments TAA;
		};
	}
}