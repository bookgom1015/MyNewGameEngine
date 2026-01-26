#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace SSCS {
		namespace Shader {
			enum Type {
				CS_ComputeContactShadow = 0,
				CS_ApplyContactShadow,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_ComputeContactShadow = 0,
				GR_ApplyContactShadow,
				Count
			};

			namespace ComputeContactShadow {
				enum {
					CB_Pass = 0,
					CB_Light,
					CB_SSCS,
					SI_PositionMap,
					SI_NormalMap,
					SI_DepthMap,
					UO_ContactShadowMap,
					UO_DebugMap,
					Count
				};
			}

			namespace ApplyContactShadow {
				enum {
					UI_ContactShadowMap = 0,
					UIO_ShadowMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				CP_ComputeContactShadow = 0,
				CP_ApplyContactShadow,
				Count
			};
		}

		class SSCSClass : public Render::DX::Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
				UINT ClientWidth{};
				UINT ClientHeight{};
			};

		public:
			SSCSClass();
			virtual ~SSCSClass();

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;
			virtual void CleanUp() override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures() override;
			virtual BOOL BuildPipelineStates() override;
			virtual BOOL BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) override;
			virtual BOOL OnResize(UINT width, UINT height) override;

		public:
			BOOL ComputeContactShadow(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				Foundation::Resource::GpuResource* const pNormalMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
				Foundation::Resource::GpuResource* const pDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap);
			BOOL ApplyContactShadow(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pShadowMap,
				D3D12_GPU_DESCRIPTOR_HANDLE uio_shadowMap);


		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData{};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures{};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			std::unique_ptr<Foundation::Resource::GpuResource> mDebugMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhDebugMapCpuUav{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhDebugMapGpuUav{};

			std::unique_ptr<Foundation::Resource::GpuResource> mContactShadowMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhContactShadowMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhContactShadowMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhContactShadowMapCpuUav{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhContactShadowMapGpuUav{};
		};

		using InitDataPtr = std::unique_ptr<SSCSClass::InitData>;

		InitDataPtr MakeInitData();
	}
}