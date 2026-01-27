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
			__forceinline ID3D11ShaderResourceView* DiffuseIrradianceMapSrv();
			__forceinline ID3D11ShaderResourceView* PrefilteredEnvironmentCubeMapSrv();
			__forceinline ID3D11ShaderResourceView* BrdfLutMapSrv();

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
			virtual void CleanUp() override;

			virtual BOOL CompileShaders();
			virtual BOOL BuildPipelineStates();

		public:
			BOOL DrawSkySphere(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D11_VIEWPORT& viewport,
				ID3D11RenderTargetView* const pBackBufferRtv,
				ID3D11DepthStencilView* const pDsv,
				Foundation::RenderItem* pSphere);

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

			Microsoft::WRL::ComPtr<ID3D11Texture2D> mEnvironmentCubeMap{};
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhEnvironmentCubeMapSrv{};

			Microsoft::WRL::ComPtr<ID3D11Texture2D> mDiffuseIrradianceCubeMap{};
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhDiffuseIrradianceCubeMapSrv{};

			Microsoft::WRL::ComPtr<ID3D11Texture2D> mPrefilteredEnvironmentCubeMap{};
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhPrefilteredEnvironmentCubeMapSrv{};

			Microsoft::WRL::ComPtr<ID3D11Texture2D> mBrdfLutMap{};
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhBrdfLutMapSrv{};
		};

		using InitDataPtr = std::unique_ptr<EnvironmentMapClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

namespace Render::DX11::Shading::EnvironmentMap {
	ID3D11ShaderResourceView* EnvironmentMapClass::DiffuseIrradianceMapSrv() {
		return mhDiffuseIrradianceCubeMapSrv.Get();
	}

	ID3D11ShaderResourceView* EnvironmentMapClass::PrefilteredEnvironmentCubeMapSrv() {
		return mhPrefilteredEnvironmentCubeMapSrv.Get();
	}

	ID3D11ShaderResourceView* EnvironmentMapClass::BrdfLutMapSrv() {
		return mhBrdfLutMapSrv.Get();
	}
}