#pragma once

#include "Common/Util/HashUtil.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX11 {
	namespace Foundation::Core {
		class Device;
	}

	namespace Shading::Util {
		class ShaderManager {
		public:
			struct D3D11ShaderInfo {
				LPCWSTR FileName{};
				D3D_SHADER_MACRO* Defines{};
				LPCSTR EntryPoint{};
				LPCSTR Target{};
				ID3D11ClassLinkage* ClassLinkage{};
			};

		public:
			ShaderManager();
			virtual ~ShaderManager();

		public:
			__forceinline ID3DBlob* GetShader(Common::Foundation::Hash hash);

		public:
			BOOL Initialize(Common::Debug::LogFile* const pLogFile, LPCWSTR baseDir);
			void CleanUp();

		public:
			BOOL CompileVertexShader(
				Foundation::Core::Device* const pDevice,
				const D3D11ShaderInfo& shaderInfo,
				Common::Foundation::Hash& hash,
				ID3D11VertexShader** ppShader);
			BOOL CompileGeometryShader(
				Foundation::Core::Device* const pDevice,
				const D3D11ShaderInfo& shaderInfo,
				Common::Foundation::Hash& hash,
				ID3D11GeometryShader** ppShader);
			BOOL CompilePixelShader(
				Foundation::Core::Device* const pDevice,
				const D3D11ShaderInfo& shaderInfo,
				Common::Foundation::Hash& hash,
				ID3D11PixelShader** ppShader);
			BOOL CompileComputeShader(
				Foundation::Core::Device* const pDevice,
				const D3D11ShaderInfo& shaderInfo,
				Common::Foundation::Hash& hash,
				ID3D11ComputeShader** ppShader);

		private:
			BOOL CompileShader(
				const D3D11ShaderInfo& shaderInfo,
				ID3DBlob** ppShaderBlob);

		private:
			BOOL mbCleanedUp{};
			Common::Debug::LogFile* mpLogFile{};
			LPCWSTR mpBaseDir{};

			std::unordered_map<Common::Foundation::Hash, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders{};
		};
	}
}

#include "ShaderManager.inl"