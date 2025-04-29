#pragma once

#pragma comment(lib, "dxcompiler.lib")

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3d12.h>
#include <dxcapi.h>

#include "Common/Util/HashUtil.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Shading::Util {
	class ShaderManager {
	public:
		struct D3D12ShaderInfo {
			LPCWSTR		FileName	  = nullptr;
			LPCWSTR		EntryPoint	  = nullptr;
			LPCWSTR		TargetProfile = nullptr;
			DxcDefine*	Defines		  = nullptr;
			UINT32		DefineCount	  = 0;

			D3D12ShaderInfo() = default;
			D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile);
			D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile, DxcDefine* defines, UINT32 defCount);
			D3D12ShaderInfo(const D3D12ShaderInfo& ref);
			~D3D12ShaderInfo();

			D3D12ShaderInfo& operator=(const D3D12ShaderInfo& ref);
		};

	public:
		ShaderManager() = default;
		virtual ~ShaderManager() = default;

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
		Common::Debug::LogFile* mpLogFile = nullptr;
		UINT mThreadCount = 0;

		std::vector<Microsoft::WRL::ComPtr<IDxcUtils>> mUtils;
		std::vector<Microsoft::WRL::ComPtr<IDxcCompiler3>> mCompilers;
		std::vector<std::unique_ptr<std::mutex>> mCompileMutexes;

		std::unordered_map<Common::Foundation::Hash, D3D12ShaderInfo> mShaderInfos;
		std::unordered_map<Common::Foundation::Hash, Microsoft::WRL::ComPtr<IDxcBlob>> mShaders;
		std::vector<std::vector<std::pair<Common::Foundation::Hash, Microsoft::WRL::ComPtr<IDxcBlob>>>> mStagingShaders;
	};
}

namespace std {
	template<>
	struct hash<Render::DX::Shading::Util::ShaderManager::D3D12ShaderInfo> {
		Common::Foundation::Hash operator()(const Render::DX::Shading::Util::ShaderManager::D3D12ShaderInfo& info) const {
			Common::Foundation::Hash hash = 0;
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.FileName));
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.EntryPoint));
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.TargetProfile));
			hash = Common::Util::HashUtil::HashCombine(hash, static_cast<UINT>(info.DefineCount));
			return hash;
		}
	};
}

#include "Render/DX/Shading/Util/ShaderManager.inl"