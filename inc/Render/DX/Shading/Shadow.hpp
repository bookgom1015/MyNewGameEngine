#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

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
					SI_FaceIdCubeMap,
					UO_ShadowMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_DrawZDepth = 0,
				GP_DrawZDepthCube,
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
			};

		public:
			ShadowClass();
			virtual ~ShadowClass() = default;

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

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		public:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures = {};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates = {};

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, MaxLights> mZDepthMaps;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthMapCpuSrvs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthMapGpuSrvs;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthMapCpuDsvs;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, MaxLights> mZDepthCubeMaps;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthCubeMapCpuSrvs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthCubeMapGpuSrvs;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxLights> mhZDepthCubeMapCpuDsvs;

			std::unique_ptr<Foundation::Resource::GpuResource> mShadowMap;
			D3D12_CPU_DESCRIPTOR_HANDLE mhShadowMapCpuSrv;
			D3D12_GPU_DESCRIPTOR_HANDLE mhShadowMapGpuSrv;
		};

		using InitDataPtr = std::unique_ptr<ShadowClass::InitData>;

		InitDataPtr MakeInitData();
	}
}