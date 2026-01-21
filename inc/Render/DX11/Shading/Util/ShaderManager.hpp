#pragma once

#include "Common/Util/HashUtil.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX11::Shading::Util {
	class ShaderManager {
	public:
		struct D3D11ShaderInfo {
			LPCWSTR		FileName{};
			LPCWSTR		EntryPoint{};
			LPCWSTR		TargetProfile{};
			DxcDefine*	Defines{};
			UINT32		DefineCount{};

			D3D11ShaderInfo() = default;
			D3D11ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile);
			D3D11ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile, DxcDefine* defines, UINT32 defCount);
			D3D11ShaderInfo(const D3D11ShaderInfo& ref);
			~D3D11ShaderInfo();

			D3D11ShaderInfo& operator=(const D3D11ShaderInfo& ref);
		};

	public:
		ShaderManager();
		virtual ~ShaderManager();

	public:
		__forceinline IDxcBlob* GetShader(Common::Foundation::Hash hash);

	public:
		BOOL Initialize(Common::Debug::LogFile* const pLogFile, UINT numThreads);
		void CleanUp();

	public:
		BOOL AddShader(const D3D11ShaderInfo& shaderInfo, Common::Foundation::Hash& hash);
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

		std::unordered_map<Common::Foundation::Hash, D3D11ShaderInfo> mShaderInfos{};
		std::unordered_map<Common::Foundation::Hash, Microsoft::WRL::ComPtr<IDxcBlob>> mShaders{};
		std::vector<std::vector<std::pair<Common::Foundation::Hash, Microsoft::WRL::ComPtr<IDxcBlob>>>> mStagingShaders{};
	};
}

#include "ShaderManager.inl"