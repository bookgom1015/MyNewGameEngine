#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace VolumetricLight {
		namespace Shader {
			enum Type {
				CS_CalculateScatteringAndDensity = 0,
				CS_AccumulateScattering,
				CS_BlendScattering,
				VS_ApplyFog,
				PS_ApplyFog,
				PS_ApplyFog_Tricubic,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_CalculateScatteringAndDensity = 0,
				GR_AccumulateScattering,
				GR_BlendScattering,
				GR_ApplyFog,
				Count
			};

			namespace CalculateScatteringAndDensity {
				enum {
					CB_Pass = 0,
					CB_Light,
					RC_Consts,
					SI_ZDepthMaps,
					SI_ZDepthCubeMaps,
					UO_FrustumVolumeMap,
					Count
				};
			}

			namespace AccumulateScattering {
				enum {
					RC_Consts = 0,
					UIO_FrustumVolumeMap,
					Count
				};
			}

			namespace BlendScattering {
				enum {
					SI_PreviousScattering = 0,
					UIO_CurrentScattering,
					Count
				};
			}

			namespace ApplyFog {
				enum {
					CB_Pass = 0,
					RC_Consts,
					SI_PositionMap,
					SI_FrustumVolumeMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				CP_CalculateScatteringAndDensity = 0,
				CP_AccumulateScattering,
				CP_BlendScattering,
				GP_ApplyFog,
				GP_ApplyFog_Tricubic,
				Count
			};
		}

		namespace Descriptor {
			enum FrustumVolumeMap {
				E_Srv = 0,
				E_Uav,
				Count
			};
		}

		class VolumetricLightClass : public Render::DX::Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
				UINT TextureWidth = 0;
				UINT TextureHeight = 0;
				UINT TextureDepth = 0;
			};

		public:
			VolumetricLightClass();
			virtual ~VolumetricLightClass() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures() override;
			virtual BOOL BuildPipelineStates() override;
			virtual BOOL BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) override;

		public:
			BOOL BuildFog(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* ppDepthMaps[],
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMaps,
				FLOAT nearZ, FLOAT farZ, FLOAT depth_exp,
				FLOAT uniformDensity, FLOAT densityScale, 
				FLOAT anisotropicCoeff,
				UINT numLights);
			BOOL ApplyFog(
				Foundation::Resource::FrameResource* const pFrameResource,				
				Foundation::Resource::GpuResource* pBackBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
				FLOAT nearZ, FLOAT farZ, FLOAT depth_exp,
				BOOL tricubicSamplingEnabled);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

			BOOL CalculateScatteringAndDensity(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* ppDepthMaps[],
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMaps,
				FLOAT nearZ, FLOAT farZ, FLOAT depth_exp,
				FLOAT uniformDensity, FLOAT anisotropicCoeff,
				UINT numLights);
			BOOL AccumulateScattering(
				Foundation::Resource::FrameResource* const pFrameResource,
				FLOAT nearZ, FLOAT farZ, FLOAT depth_exp, FLOAT densityScale);
			BOOL BlendScattering(Foundation::Resource::FrameResource* const pFrameResource);

		private:
			InitData mInitData;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, 2> mFrustumVolumeMaps;
			std::array<std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2>, Descriptor::FrustumVolumeMap::Count> mhFrustumVolumeMapCpus;
			std::array<std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2>, Descriptor::FrustumVolumeMap::Count> mhFrustumVolumeMapGpus;

			UINT mFrameCount = 0;
			UINT mCurrentFrame = 0;
			UINT mPreviousFrame = 0;
		};

		using InitDataPtr = std::unique_ptr<VolumetricLightClass::InitData>;

		InitDataPtr MakeInitData();
	}
}