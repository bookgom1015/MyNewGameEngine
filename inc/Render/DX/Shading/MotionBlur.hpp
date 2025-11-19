#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace MotionBlur {
		namespace Shader {
			enum Type {
				VS_MotionBlur = 0,
				MS_MotionBlur,
				PS_MotionBlur,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_Default = 0,
				Count
			};

			namespace Default {
				enum {
					RC_Consts = 0,
					SI_BackBuffer,
					SI_DepthMap,
					SI_VelocityMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_MotionBlur = 0,
				MP_MotionBlur,
				Count
			};
		}

		class MotionBlurClass : public Foundation::ShadingObject {
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
			MotionBlurClass();
			virtual ~MotionBlurClass() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) override;
			virtual BOOL BuildPipelineStates() override;

		public:
			BOOL ApplyMotionBlur(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D12_VIEWPORT& viewport,
				const D3D12_RECT& scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const pBackBufferCopy,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
				Foundation::Resource::GpuResource* const pDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
				Foundation::Resource::GpuResource* const pVelocityMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap,
				FLOAT intensity, FLOAT limit, FLOAT depthBias, INT sampleCount);

		private:
			InitData mInitData;

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;
		};

		using InitDataPtr = std::unique_ptr<MotionBlurClass::InitData>;

		InitDataPtr MakeInitData();
	}
}