#pragma once

#include "Common/Util/HashUtil.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX11::Shading::Util {
	class ShaderManager {
	public:
		struct D3D11ShaderInfo {
			LPCSTR FileName{};
			D3D_SHADER_MACRO* Defines{};
			LPCSTR EntryPoint{};
			LPCSTR Target{};
		};

	public:
		ShaderManager();
		virtual ~ShaderManager();

	public:
		__forceinline ID3DBlob* GetShader(Common::Foundation::Hash hash);

	public:
		BOOL Initialize(Common::Debug::LogFile* const pLogFile);
		void CleanUp();

	public:
		BOOL AddShader(const D3D11ShaderInfo& shaderInfo, Common::Foundation::Hash& hash);
		BOOL CompileShaders(LPCWSTR baseDir);

	private:
		BOOL CompileShader(
			LPCWSTR filePath,
			D3D_SHADER_MACRO* defines,
			LPCSTR entryPoint,
			LPCSTR target,
			ID3DBlob** ppShaderByteCode);

	private:
		Common::Debug::LogFile* mpLogFile{};

		std::unordered_map<Common::Foundation::Hash, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders{};
	};
}

#include "ShaderManager.inl"