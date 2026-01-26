#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11::Shading {
	namespace GammaCorrection {
		namespace Shader {
			enum Type {
				VS_GammaCorrection = 0,
				PS_GammaCorrection,
				Count
			};
		}

		class GammaCorrectionClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				UINT Width{};
				UINT Height{};
				Foundation::Core::Device* Device{};
				Util::ShaderManager* ShaderManager{};
			};

		public:
			GammaCorrectionClass();
			virtual ~GammaCorrectionClass();

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
			virtual void CleanUp() override;

			virtual BOOL CompileShaders();
			virtual BOOL BuildPipelineStates();

		public:
			BOOL Apply(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D11_VIEWPORT& viewport,
				ID3D11Texture2D* const pBackBuffer,
				ID3D11RenderTargetView* const pBackBufferRtv,
				ID3D11Texture2D* const pBackBufferCopy,
				ID3D11ShaderResourceView* const pBackBufferCopySrv);

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
			Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};

			Microsoft::WRL::ComPtr<ID3D11VertexShader> mGammaCorrectionVS{};
			Microsoft::WRL::ComPtr<ID3D11PixelShader> mGammaCorrectionPS{};
		};

		using InitDataPtr = std::unique_ptr<GammaCorrectionClass::InitData>;

		InitDataPtr MakeInitData();
	}
}