#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11::Shading {
	namespace ToneMapping {
		namespace Shader {
			enum Type {
				VS_ToneMapping = 0,
				PS_ToneMapping,
				Count
			};
		}

		class ToneMappingClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				UINT Width{};
				UINT Height{};
				Foundation::Core::Device* Device{};
				Util::ShaderManager* ShaderManager{};
			};

		public:
			ToneMappingClass();
			virtual ~ToneMappingClass();

		public:
			__forceinline ID3D11Texture2D* InterMediateMap();
			__forceinline ID3D11ShaderResourceView* InterMediateMapSrv();
			__forceinline ID3D11RenderTargetView* InterMediateMapRtv();

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
			virtual void CleanUp() override;

			virtual BOOL CompileShaders();
			virtual BOOL BuildPipelineStates();
			virtual BOOL OnResize(UINT width, UINT height);

		public:
			BOOL Apply(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D11_VIEWPORT& viewport,
				ID3D11Texture2D* const pBackBuffer,
				ID3D11RenderTargetView* const pBackBufferRtv);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
			Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};

			Microsoft::WRL::ComPtr<ID3D11VertexShader> mToneMappingVS{};
			Microsoft::WRL::ComPtr<ID3D11PixelShader> mToneMappingPS{};

			Microsoft::WRL::ComPtr<ID3D11Texture2D> mIntermediateMap{};
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhIntermediateMapSrv{};
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mhIntermediateMapRtv{};
		};

		using InitDataPtr = std::unique_ptr<ToneMappingClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "ToneMapping.inl"