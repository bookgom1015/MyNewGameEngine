#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11::Shading {
	namespace EnvironmentMap {
		namespace Shader {
			enum Type {
				VS_DrawSkySphere = 0,
				PS_DrawSkySphere,
				Count
			};
		}

		class EnvironmentMapClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				UINT Width{};
				UINT Height{};
				Foundation::Core::Device* Device{};
				Util::ShaderManager* ShaderManager{};
			};

		public:
			EnvironmentMapClass();
			virtual ~EnvironmentMapClass();

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
			virtual void CleanUp() override;

			virtual BOOL CompileShaders();
			virtual BOOL BuildPipelineStates();

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
			Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};

			Microsoft::WRL::ComPtr<ID3D11VertexShader> mDrawSkySphereVS{};
			Microsoft::WRL::ComPtr<ID3D11PixelShader> mDrawSkySpherePS{};
			Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
		};

		using InitDataPtr = std::unique_ptr<EnvironmentMapClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

