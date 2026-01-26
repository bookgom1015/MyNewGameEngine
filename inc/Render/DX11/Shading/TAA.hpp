#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11::Shading {
	namespace TAA {
		namespace Shader {
			enum Type {
				VS_TAA = 0,
				PS_TAA,
				Count
			};
		}

		class TAAClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				UINT Width{};
				UINT Height{};
				Foundation::Core::Device* Device{};
				Util::ShaderManager* ShaderManager{};
			};

		public:
			TAAClass();
			virtual ~TAAClass();

		public:
			__forceinline constexpr UINT HaltonSequenceSize() const;
			__forceinline constexpr DirectX::XMFLOAT2 HaltonSequence(UINT index) const;

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
				ID3D11RenderTargetView* const pBackBufferRtv,
				ID3D11Texture2D* const pBackBufferCopy,
				ID3D11ShaderResourceView* const pBackBufferCopySrv,
				ID3D11ShaderResourceView* const pVelocityMapSrv);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
			Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};

			Microsoft::WRL::ComPtr<ID3D11VertexShader> mTaaVS{};
			Microsoft::WRL::ComPtr<ID3D11PixelShader> mTaaPS{};

			Microsoft::WRL::ComPtr<ID3D11Texture2D> mHistoryMap{};
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhHistoryMapSrv{};

			const std::array<DirectX::XMFLOAT2, 16> mHaltonSequence = {
						DirectX::XMFLOAT2(0.5f, 0.333333f),
						DirectX::XMFLOAT2(0.25f, 0.666667f),
						DirectX::XMFLOAT2(0.75f, 0.111111f),
						DirectX::XMFLOAT2(0.125f, 0.444444f),
						DirectX::XMFLOAT2(0.625f, 0.777778f),
						DirectX::XMFLOAT2(0.375f, 0.222222f),
						DirectX::XMFLOAT2(0.875f, 0.555556f),
						DirectX::XMFLOAT2(0.0625f, 0.888889f),
						DirectX::XMFLOAT2(0.5625f, 0.037037f),
						DirectX::XMFLOAT2(0.3125f, 0.37037f),
						DirectX::XMFLOAT2(0.8125f, 0.703704f),
						DirectX::XMFLOAT2(0.1875f, 0.148148f),
						DirectX::XMFLOAT2(0.6875f, 0.481481f),
						DirectX::XMFLOAT2(0.4375f, 0.814815f),
						DirectX::XMFLOAT2(0.9375f, 0.259259f),
						DirectX::XMFLOAT2(0.03125f, 0.592593f)
			};
			std::array<DirectX::XMFLOAT2, 16> mFittedToBakcBufferHaltonSequence{};
		};

		using InitDataPtr = std::unique_ptr<TAAClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "TAA.inl"