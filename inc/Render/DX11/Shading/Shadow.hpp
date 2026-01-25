#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11::Shading {
	namespace Shadow {
		namespace Shader {
			enum Type {
				VS_DrawZDepth = 0,
				GS_DrawZDepth,
				PS_DrawZDepth,
				CS_DrawShadow,
				Count
			};
		}

		class ShadowClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				UINT ClientWidth{};
				UINT ClientHeight{};
				UINT TextureWidth{};
				UINT TextureHeight{};
				Foundation::Core::Device* Device{};
				Util::ShaderManager* ShaderManager{};
			};

		public:
			ShadowClass();
			virtual ~ShadowClass();

		public:
			__forceinline ID3D11ShaderResourceView* ShadowMapSrv();
			__forceinline ID3D11UnorderedAccessView* ShadowMapUav();

			__forceinline constexpr UINT LightCount() const noexcept;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
			virtual void CleanUp() override;

			virtual BOOL CompileShaders();
			virtual BOOL BuildPipelineStates();
			virtual BOOL OnResize(UINT width, UINT height);

		public:
			BOOL AddLight(UINT lightType);

		public:
			BOOL Run(
				Foundation::Resource::FrameResource* const pFrameResource,
				ID3D11ShaderResourceView* pPositionMapSrv,
				Foundation::RenderItem** ppRitems,
				UINT numRitems);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

			BOOL BuildResources(BOOL bCube);
			BOOL BuildDescriptors(BOOL bCube);

			BOOL DrawZDepth(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::RenderItem** ppRitems,
				UINT numRitems);
			BOOL DrawShadow(
				Foundation::Resource::FrameResource* const pFrameResource,
				ID3D11ShaderResourceView* pPositionMapSrv);

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
			Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};

			Microsoft::WRL::ComPtr<ID3D11VertexShader> mDrawZDepthVS{};
			Microsoft::WRL::ComPtr<ID3D11GeometryShader> mDrawZDepthGS{};
			Microsoft::WRL::ComPtr<ID3D11PixelShader> mDrawZDepthPS{};
			Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};

			Microsoft::WRL::ComPtr<ID3D11ComputeShader> mDrawShadowCS{};

			std::array<Microsoft::WRL::ComPtr<ID3D11Texture2D>, MAX_LIGHTS> mZDepthMaps{};
			std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, MAX_LIGHTS> mhZDepthMapSrvs{};
			std::array<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>, MAX_LIGHTS> mhZDepthMapDsvs{};

			Microsoft::WRL::ComPtr<ID3D11Texture2D> mShadowMap{};
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhShadowMapSrv{};
			Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mhShadowMapUav{};

			UINT mLightCount{};

			D3D11_VIEWPORT mViewport{};
		};

		using InitDataPtr = std::unique_ptr<ShadowClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "Shadow.inl"