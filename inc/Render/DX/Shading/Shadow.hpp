#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"
#include "Render/DX/Foundation/Light.h"

namespace Render::DX::Foundation {
	struct Light;
}

namespace Render::DX::Shading {
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

		namespace RootSignature {
			enum Type {
				GR_DrawZDepth = 0,
				GR_DrawShadow,
				Count
			};

			namespace DrawZDepth {
				enum {
					CB_Pass = 0,
					CB_Object,
					CB_Material,
					RC_Consts,
					SI_Textures,
					Count
				};
			}

			namespace DrawShadow {
				enum {
					CB_Pass = 0,
					RC_Consts,
					SI_PositionMap,
					SI_ZDepthMap,
					SI_ZDepthCubeMap,
					UIO_ShadowMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_DrawZDepth = 0,
				CP_DrawShadow,
				Count
			};
		}

		class ShadowClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL MeshShaderSupported = FALSE;
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
				UINT ClientWidth;
				UINT ClientHeight;
				UINT TexWidth;
				UINT TexHeight;
			};

		public:
			ShadowClass();
			virtual ~ShadowClass() = default;

		public:
			__forceinline constexpr const Foundation::Light* Lights() const;
			__forceinline constexpr Foundation::Light Light(UINT index) const;
			__forceinline constexpr UINT LightCount() const;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) override;
			virtual BOOL BuildPipelineStates() override;
			virtual BOOL BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) override;
			virtual BOOL OnResize(UINT width, UINT height) override;

		public:
			BOOL Run(
				Foundation::Resource::FrameResource* const pFrameResource, 
				const std::vector<Render::DX::Foundation::RenderItem*>& ritems);

			BOOL AddLight(const Foundation::Light& light);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

			BOOL BuildResource(BOOL bCube);
			BOOL BuildDescriptor(BOOL bCube);

			BOOL DrawZDepth(
				Foundation::Resource::FrameResource* const pFrameResource,
				const std::vector<Render::DX::Foundation::RenderItem*>& ritems,
				UINT lightIndex);
			BOOL DrawRenderItems(
				Foundation::Resource::FrameResource* const pFrameResource,
				ID3D12GraphicsCommandList6* const pCmdList,
				const std::vector<Render::DX::Foundation::RenderItem*>& ritems);

		public:
			InitData mInitData;

			D3D12_VIEWPORT mViewport;
			D3D12_RECT mScissorRect;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, MaxLights> mZDepthMaps;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthMapCpuSrvs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthMapGpuSrvs;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthMapCpuDsvs;

			std::unique_ptr<Foundation::Resource::GpuResource> mShadowMap;
			D3D12_CPU_DESCRIPTOR_HANDLE mhShadowMapCpuSrv;
			D3D12_GPU_DESCRIPTOR_HANDLE mhShadowMapGpuSrv;
			D3D12_CPU_DESCRIPTOR_HANDLE mhShadowMapCpuUav;
			D3D12_GPU_DESCRIPTOR_HANDLE mhShadowMapGpuUav;

			// Lights
			std::array<Foundation::Light, MaxLights> mLights;
			UINT mLightCount = 0;
		};

		using InitDataPtr = std::unique_ptr<ShadowClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

constexpr const Render::DX::Foundation::Light* Render::DX::Shading::Shadow::ShadowClass::Lights() const {
	return mLights.data();
}

constexpr Render::DX::Foundation::Light Render::DX::Shading::Shadow::ShadowClass::Light(UINT index) const {
	return mLights[index];
}

constexpr UINT Render::DX::Shading::Shadow::ShadowClass::LightCount() const {
	return mLightCount;
}