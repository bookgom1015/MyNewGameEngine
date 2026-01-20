#pragma once

#include "Common/Util/HashUtil.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Shading::Util {
	class ShaderManager {
	public:
		struct D3D12ShaderInfo {
			LPCWSTR		FileName{};
			LPCWSTR		EntryPoint{};
			LPCWSTR		TargetProfile{};
			DxcDefine*	Defines{};
			UINT32		DefineCount{};

			D3D12ShaderInfo() = default;
			D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile);
			D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile, DxcDefine* defines, UINT32 defCount);
			D3D12ShaderInfo(const D3D12ShaderInfo& ref);
			~D3D12ShaderInfo();

			D3D12ShaderInfo& operator=(const D3D12ShaderInfo& ref);
		};

	public:
		ShaderManager();
		virtual ~ShaderManager();

	public:
		__forceinline IDxcBlob* GetShader(Common::Foundation::Hash hash);

	public:
		BOOL Initialize(Common::Debug::LogFile* const pLogFile, UINT numThreads);

		BOOL AddShader(const D3D12ShaderInfo& shaderInfo, Common::Foundation::Hash& hash);
		BOOL CompileShaders(LPCWSTR baseDir);

	private:
		BOOL CompileShader(Common::Foundation::Hash hash, LPCWSTR baseDir);
		BOOL CommitShaders();
		BOOL BuildPdb(IDxcResult* const result, LPCWSTR fileName);

	private:
		Common::Debug::LogFile* mpLogFile{};
		UINT mThreadCount{};

		std::vector<Microsoft::WRL::ComPtr<IDxcUtils>> mUtils{};
		std::vector<Microsoft::WRL::ComPtr<IDxcCompiler3>> mCompilers{};
		std::vector<std::unique_ptr<std::mutex>> mCompileMutexes{};

		std::unordered_map<Common::Foundation::Hash, D3D12ShaderInfo> mShaderInfos{};
		std::unordered_map<Common::Foundation::Hash, Microsoft::WRL::ComPtr<IDxcBlob>> mShaders{};
		std::vector<std::vector<std::pair<Common::Foundation::Hash, Microsoft::WRL::ComPtr<IDxcBlob>>>> mStagingShaders{};
	};
}

#include "Render/DX/Shading/Util/ShaderManager.inl"