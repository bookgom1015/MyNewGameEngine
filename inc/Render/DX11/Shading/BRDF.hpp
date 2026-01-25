#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11::Shading {
	namespace BRDF {
		namespace Shader {
			enum Type {
				VS_ComputeBRDF = 0,
				PS_ComputeBRDF,
				Count
			};
		}

		class BRDFClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				UINT Width{};
				UINT Height{};
				Foundation::Core::Device* Device{};
				Util::ShaderManager* ShaderManager{};
			};

		public:
			BRDFClass();
			virtual ~BRDFClass();

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
			virtual void CleanUp() override;

			virtual BOOL CompileShaders();
			virtual BOOL BuildPipelineStates();

		public:
			BOOL ComputeBRDF(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D11_VIEWPORT& viewport,
				ID3D11RenderTargetView* pBackBufferRtv,
				ID3D11ShaderResourceView* pDiffuseMapSrv,
				ID3D11ShaderResourceView* pNormalMapSrv,
				ID3D11ShaderResourceView* pPositionMapSrv,
				ID3D11ShaderResourceView* pRoughnessMetalnessMapSrv,
				ID3D11ShaderResourceView* pShadowMapSrv,
				UINT numLights);

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
			Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};

			Microsoft::WRL::ComPtr<ID3D11VertexShader> mBRDFVS{};
			Microsoft::WRL::ComPtr<ID3D11PixelShader> mBRDFPS{};
		};

		using InitDataPtr = std::unique_ptr<BRDFClass::InitData>;

		InitDataPtr MakeInitData();
	}
}