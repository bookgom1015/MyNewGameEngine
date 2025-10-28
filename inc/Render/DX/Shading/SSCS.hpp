#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace SSCS {
		namespace Shader {
			enum Type {
				CS_ComputeContactShadow = 0,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_ComputeContactShadow = 0,
				Count
			};

			namespace ComputeContactShadow {
				enum {
					CB_Pass = 0,
					CB_Light,
					RC_Consts,
					SI_PositionMap,
					SI_DepthMap,
					UO_ContactShadowMap,
					UO_DebugMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				CP_ComputeContactShadow = 0,
				Count
			};
		}

		class SSCSClass : public Render::DX::Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
				UINT ClientWidth = 0;
				UINT ClientHeight = 0;
			};

		public:
			SSCSClass();
			virtual ~SSCSClass() = default;

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
			BOOL ComputeContactShadow(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				Foundation::Resource::GpuResource* const pDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::unique_ptr<Foundation::Resource::GpuResource> mDebugMap;
			D3D12_CPU_DESCRIPTOR_HANDLE mhDebugMapCpuUav;
			D3D12_GPU_DESCRIPTOR_HANDLE mhDebugMapGpuUav;

			std::unique_ptr<Foundation::Resource::GpuResource> mContactShadowMap;
			D3D12_CPU_DESCRIPTOR_HANDLE mhContactShadowMapCpuSrv;
			D3D12_GPU_DESCRIPTOR_HANDLE mhContactShadowMapGpuSrv;
			D3D12_CPU_DESCRIPTOR_HANDLE mhContactShadowMapCpuUav;
			D3D12_GPU_DESCRIPTOR_HANDLE mhContactShadowMapGpuUav;
		};

		using InitDataPtr = std::unique_ptr<SSCSClass::InitData>;

		InitDataPtr MakeInitData();
	}
}