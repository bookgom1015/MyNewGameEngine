#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11 {
	namespace Shading {
		namespace GBuffer {
			namespace Shader {
				enum Type {
					VS_GBuffer = 0,
					PS_GBuffer,
					Count
				};
			}

			namespace Resource {
				enum Type {
					E_Albedo = 0,
					E_Normal,
					E_Position,
					E_RoughnessMetalness,
					Count
				};
			}

			namespace Descriptor {
				namespace Srv {
					enum Type {
						E_Albedo = 0,
						E_Normal,
						E_Position,
						E_RoughnessMetalness,
						Count
					};
				}

				namespace Rtv {
					enum Type {
						E_Albedo = 0,
						E_Normal,
						E_Position,
						E_RoughnessMetalness,
						Count
					};
				}
			}

			class GBufferClass : public Foundation::ShadingObject {
			public:
				struct InitData {
					UINT Width{};
					UINT Height{};
					Foundation::Core::Device* Device{};
					Util::ShaderManager* ShaderManager{};
				};

			public:
				GBufferClass();
				virtual ~GBufferClass();

			public:
				__forceinline ID3D11ShaderResourceView* AlbedoMapSrv();
				__forceinline ID3D11ShaderResourceView* NormalMapSrv();
				__forceinline ID3D11ShaderResourceView* PositionMapSrv();
				__forceinline ID3D11ShaderResourceView* RoughnessMetalnessMapSrv();

			public:
				virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
				virtual void CleanUp() override;

				virtual BOOL CompileShaders();
				virtual BOOL BuildPipelineStates();
				virtual BOOL OnResize(UINT width, UINT height);

			private:
				BOOL BuildResources();
				BOOL BuildDescriptors();

			public:
				BOOL DrawGBuffer(
					Foundation::Resource::FrameResource* const pFrameResource,
					const D3D11_VIEWPORT& viewport,
					ID3D11DepthStencilView* const pDsv,
					Foundation::RenderItem** ppRitems,
					UINT numRitems);

			private:
				InitData mInitData{};

				std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

				Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
				Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
				Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};

				std::array<Microsoft::WRL::ComPtr<ID3D11Texture2D>, Resource::Count> mResources{};
				std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, Descriptor::Srv::Count> mhSrvs{};
				std::array<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>, Descriptor::Rtv::Count> mhRtvs{};				

				Microsoft::WRL::ComPtr<ID3D11VertexShader> mGBufferVS{};
				Microsoft::WRL::ComPtr<ID3D11PixelShader> mGBufferPS{};
				Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
			};

			using InitDataPtr = std::unique_ptr<GBufferClass::InitData>;

			InitDataPtr MakeInitData();
		}
	}
}

namespace Render::DX11::Shading::GBuffer {
	__forceinline ID3D11ShaderResourceView* GBufferClass::AlbedoMapSrv() {
		return mhSrvs[Descriptor::Srv::E_Albedo].Get();
	}

	__forceinline ID3D11ShaderResourceView* GBufferClass::NormalMapSrv() {
		return mhSrvs[Descriptor::Srv::E_Normal].Get();
	}

	__forceinline ID3D11ShaderResourceView* GBufferClass::PositionMapSrv() {
		return mhSrvs[Descriptor::Srv::E_Position].Get();
	}

	__forceinline ID3D11ShaderResourceView* GBufferClass::RoughnessMetalnessMapSrv() {
		return mhSrvs[Descriptor::Srv::E_RoughnessMetalness].Get();
	}
}