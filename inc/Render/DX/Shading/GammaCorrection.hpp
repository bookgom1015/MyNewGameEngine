#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace Shader {
		enum Type {
			VS_GammaCorrect = 0,
			MS_GammaCorrect,
			PS_GammaCorrect,
			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				RC_Consts = 0,
				SI_BackBuffer,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_GammaCorrect = 0,
			MP_GammaCorrect,
			Count
		};
	}

	namespace GammaCorrection {
		class GammaCorrectionClass : public Foundation::ShadingObject {
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
			GammaCorrectionClass();
			virtual ~GammaCorrectionClass() = default;

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
			BOOL ApplyCorrection(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D12_VIEWPORT& viewport,
				const D3D12_RECT& scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const pBackBufferCopy,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
				FLOAT gamma);

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;
		};

		using InitDataPtr = std::unique_ptr<GammaCorrectionClass::InitData>;

		InitDataPtr MakeInitData();
	}
}