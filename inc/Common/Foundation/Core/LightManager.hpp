#pragma once

#include <array>
#include <memory>

#include "Common/Foundation/Light.h"

namespace Common{
	namespace Debug {
		struct LogFile;
	}

	namespace Foundation::Core {
		class LightManager {
		public:
			LightManager();
			virtual ~LightManager();

		public:
			bool Initialize(Debug::LogFile* const pLogFile);
			void CleanUp();

		public:
			bool AddLight(const Light& light);
			bool AddLight(LightType type);

		private:
			bool mpCleanedUp{};

			Debug::LogFile* mpLogFile{};

			std::array<std::unique_ptr<Light>, MaxLights> mLights{};
			unsigned mLightCount{};
		};
	}
}