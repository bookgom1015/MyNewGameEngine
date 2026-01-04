#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace BRDF {
		namespace Shader {
			enum Type {
				VS_ComputeBRDF = 0,
				MS_ComputeBRDF,
				PS_ComputeBRDF_BlinnPhong,
				PS_ComputeBRDF_CookTorrance,
				VS_IntegrateIrradiance,
				MS_IntegrateIrradiance,
				PS_IntegrateIrradiance,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_ComputeBRDF = 0,
				GR_IntegrateIrradiance,
				Count
			};

			namespace ComputeBRDF {
				enum {
					CB_Pass = 0,
					CB_Light,
					RC_Consts,
					SI_AlbedoMap,
					SI_NormalMap,
					SI_DepthMap,
					SI_SpecularMap,
					SI_RoughnessMetalicMap,
					SI_PositionMap,
					SI_ShadowMap,
					Count
				};
			}
			namespace IntegrateIrradiance {
				enum {
					CB_Pass = 0,
					RC_Consts,
					SI_BackBuffer,
					SI_AlbedoMap,
					SI_NormalMap,
					SI_DepthMap,
					SI_SpecularMap,
					SI_RoughnessMetalicMap,
					SI_PositionMap,
					SI_AOMap,
					SI_ReflectionMap,
					SI_DiffuseIrradianceCubeMap,
					SI_BrdfLutMap,
					SI_PrefilteredEnvCubeMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_ComputeBRDF_BlinnPhong = 0,
				MP_ComputeBRDF_BlinnPhong,
				GP_ComputeBRDF_CookTorrance,
				MP_ComputeBRDF_CookTorrance,
				GP_IntegrateIrradiance,
				MP_IntegrateIrradiance,
				Count
			};
		}

		class BRDFClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL MeshShaderSupported = FALSE;
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
				UINT ClientWidth = 0;
				UINT ClientHeight = 0;
			};

		public:
			BRDFClass();
			virtual ~BRDFClass() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures() override;
			virtual BOOL BuildPipelineStates() override;

		public:
			BOOL ComputeBRDF(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const pAlbedoMap, D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap,
				Foundation::Resource::GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap, 
				Foundation::Resource::GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap, 
				Foundation::Resource::GpuResource* const pSpecularMap, D3D12_GPU_DESCRIPTOR_HANDLE si_specularMap, 
				Foundation::Resource::GpuResource* const pRoughnessMetalnessMap, D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap,
				Foundation::Resource::GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				Foundation::Resource::GpuResource* const pShadowMap, D3D12_GPU_DESCRIPTOR_HANDLE si_shadowMap,
				BOOL bShadowEnabled);
			BOOL IntegrateIrradiance(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
				Foundation::Resource::GpuResource* const pAlbedoMap, D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap,
				Foundation::Resource::GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
				Foundation::Resource::GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
				Foundation::Resource::GpuResource* const pSpecularMap, D3D12_GPU_DESCRIPTOR_HANDLE si_specularMap,
				Foundation::Resource::GpuResource* const pRoughnessMetalnessMap, D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap,
				Foundation::Resource::GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				Foundation::Resource::GpuResource* const pAOMap, D3D12_GPU_DESCRIPTOR_HANDLE si_aoMap,
				Foundation::Resource::GpuResource* const pDiffuseIrradianceMap, D3D12_GPU_DESCRIPTOR_HANDLE si_diffuseIrradianceMap,
				Foundation::Resource::GpuResource* const pBrdfLutMap, D3D12_GPU_DESCRIPTOR_HANDLE si_brdfLutMap,
				Foundation::Resource::GpuResource* const pPrefilteredEnvCubeMap, D3D12_GPU_DESCRIPTOR_HANDLE si_prefilteredEnvCubeMap, 
				BOOL bAoEnabled);

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;
		};

		using InitDataPtr = std::unique_ptr<BRDFClass::InitData>;

		InitDataPtr MakeInitData();
	}
}