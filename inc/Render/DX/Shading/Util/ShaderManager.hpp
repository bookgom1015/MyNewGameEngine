#pragma once

#pragma comment(lib, "dxcompiler.lib")

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <wrl.h>
#include <Windows.h>

#include <d3d12.h>
#include <d3d12shader.h>
#include <dxcapi.h>

#include "Common/Foundation/Util/HashUtil.hpp"

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
			LPCWSTR*	Arguments	  = nullptr;
			DxcDefine*	Defines		  = nullptr;
			UINT32		ArgCount	  = 0;
			UINT32		DefineCount	  = 0;

			D3D12ShaderInfo() = default;
			D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile);
			D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile, DxcDefine* defines, UINT32 defCount);
		};

	public:
		ShaderManager() = default;
		virtual ~ShaderManager() = default;

	public:
		__forceinline IDxcBlob* GetShader(Common::Foundation::Hash hash);

	public:
		BOOL Initialize(Common::Debug::LogFile* const pLogFile, UINT numThreads);

		void AddShader(const D3D12ShaderInfo& shaderInfo, Common::Foundation::Hash& hash);
		BOOL CompileShaders(LPCWSTR baseDir);

	private:
		BOOL CompileShader(Common::Foundation::Hash hash, LPCWSTR baseDir);
		BOOL BuildPdb(IDxcResult* const result, LPCWSTR fileName);

	private:
		Common::Debug::LogFile* mpLogFile = nullptr;
		UINT mThreadCount = 0;

		std::vector<Microsoft::WRL::ComPtr<IDxcUtils>> mUtils;
		std::vector<Microsoft::WRL::ComPtr<IDxcCompiler3>> mCompilers;
		std::vector<std::unique_ptr<std::mutex>> mCompileMutexes;

		std::unordered_map<Common::Foundation::Hash, D3D12ShaderInfo> mShaderInfos;
		std::unordered_map<Common::Foundation::Hash, Microsoft::WRL::ComPtr<IDxcBlob>> mShaders;
	};
}

namespace std {
	template<>
	struct hash<Render::DX::Shading::Util::ShaderManager::D3D12ShaderInfo> {
		Common::Foundation::Hash operator()(const Render::DX::Shading::Util::ShaderManager::D3D12ShaderInfo& info) const {
			Common::Foundation::Hash hash = 0;
			hash = Common::Foundation::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.FileName));
			hash = Common::Foundation::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.EntryPoint));
			hash = Common::Foundation::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.TargetProfile));
			hash = Common::Foundation::Util::HashUtil::HashCombine(hash, static_cast<UINT>(info.ArgCount));
			hash = Common::Foundation::Util::HashUtil::HashCombine(hash, static_cast<UINT>(info.DefineCount));
			return hash;
		}
	};
}

#include "Render/DX/Shading/Util/ShaderManager.inl"