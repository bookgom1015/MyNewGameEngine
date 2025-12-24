#pragma once

#pragma comment(lib, "glslang.lib")

#include <memory>
#include <mutex>
#include <unordered_map>

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "Common/Util/HashUtil.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK {
	namespace Foundation::Core {
		class Device;
	}

	namespace Shading::Util {
		class ShaderManager {
		public:
			enum ShaderStage {
				E_VertexStage,
				E_FragmentStage,
			};

			struct VkShaderInfo {
				LPCSTR FileName = nullptr;
				LPCSTR EntryPoint = nullptr;
				ShaderStage Stage;
			};

		public:
			ShaderManager() = default;
			virtual ~ShaderManager();

		public:
			BOOL Initialize(Common::Debug::LogFile* const pLogFile, UINT numThreads);
			void CleanUp();

			BOOL AddShader(
				const VkShaderInfo& shaderInfo,
				Common::Foundation::Hash& hash);
			BOOL CompileShaders(const Foundation::Core::Device& device, LPCWSTR baseDir);

		private:
			BOOL ReadFile(
				Common::Foundation::Hash hash,
				LPCWSTR baseDir,
				std::vector<CHAR>& data);
			BOOL ConvertGLSLtoSPIRV(
				const VkShaderInfo& shaderInfo,
				LPCSTR pCodeData,
				const std::vector<LPCSTR>& includeDirs,
				std::vector<UINT>& spirv);
			BOOL CompileShader(
				const Foundation::Core::Device& device, 
				Common::Foundation::Hash hash, 
				LPCWSTR baseDir);
			BOOL CommitShaders();

		private:
			BOOL mbInitialized = FALSE;

			Common::Debug::LogFile* mpLogFile = nullptr;
			UINT mThreadCount = 0;

			std::vector<std::unique_ptr<std::mutex>> mCompileMutexes;

			std::unordered_map<Common::Foundation::Hash, VkShaderInfo> mShaderInfos;
			std::unordered_map<Common::Foundation::Hash, VkShaderModule> mShaders;
			std::vector<std::vector<std::pair<Common::Foundation::Hash, VkShaderModule>>> mStagingShaders;
		};
	}
}

namespace std {
	template<>
	struct hash<Render::VK::Shading::Util::ShaderManager::VkShaderInfo> {
		Common::Foundation::Hash operator()(const Render::VK::Shading::Util::ShaderManager::VkShaderInfo& info) const {
			Common::Foundation::Hash hash = 0;
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCSTR>()(info.FileName));
			return hash;
		}
	};
}