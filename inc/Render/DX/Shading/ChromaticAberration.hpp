#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace ChromaticAberration {
		namespace Shader {
			enum Type {
				VS_ChromaticAberration = 0,
				MS_ChromaticAberration,
				PS_ChromaticAberration,
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
				GP_ChromaticAberration = 0,
				MP_ChromaticAberration,
				Count
			};
		}

		class ChromaticAberrationClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL MeshShaderSupported{};
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
				UINT ClientWidth{};
				UINT ClientHeight{};
			};

		public:
			ChromaticAberrationClass();
			virtual ~ChromaticAberrationClass();

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

		public:
			BOOL ApplyChromaticAberration(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D12_VIEWPORT& viewport,
				const D3D12_RECT& scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const pBackBufferCopy,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
				FLOAT strength, FLOAT threshold, FLOAT feather,
				UINT maxShiftPx, FLOAT exponent);

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature{};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates{};
		};

		using InitDataPtr = std::unique_ptr<ChromaticAberrationClass::InitData>;

		InitDataPtr MakeInitData();
	}
}