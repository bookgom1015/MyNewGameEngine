#pragma once

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
			struct VkShaderInfo {
				LPCSTR FileName{};
				LPCSTR EntryPoint{};

				VkShaderInfo();
				VkShaderInfo(LPCSTR fileName, LPCSTR entryPoint);
			};

		public:
			ShaderManager();
			virtual ~ShaderManager();

		public:
			__forceinline VkShaderModule GetShader(Common::Foundation::Hash hash);

		public:
			BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				Foundation::Core::Device* const pDevice,
				UINT numThreads);
			void CleanUp();

			BOOL AddShader(
				const VkShaderInfo& shaderInfo,
				Common::Foundation::Hash& hash);
			BOOL CompileShaders(LPCWSTR baseDir);

		private:
			BOOL ReadFile(
				Common::Foundation::Hash hash,
				LPCWSTR baseDir,
				std::vector<CHAR>& data);
			BOOL CompileShader(
				Common::Foundation::Hash hash, 
				LPCWSTR baseDir);
			BOOL CommitShaders();

		private:
			Common::Debug::LogFile* mpLogFile{};
			Foundation::Core::Device* mpDevice{};
			UINT mThreadCount{};

			std::vector<std::unique_ptr<std::mutex>> mCompileMutexes{};

			std::unordered_map<Common::Foundation::Hash, VkShaderInfo> mShaderInfos{};
			std::unordered_map<Common::Foundation::Hash, VkShaderModule> mShaders{};
			std::vector<std::vector<std::pair<Common::Foundation::Hash, VkShaderModule>>> mStagingShaders{};
		};
	}
}

#include "ShaderManager.inl"