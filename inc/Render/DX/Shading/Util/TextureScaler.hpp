#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading::Util {
	namespace TextureScaler {
		namespace Shader {
			enum Type {
				CS_DownSample2x2 = 0,
				CS_DownSample4x4,
				CS_DownSample6x6,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_DownSample2Nx2N = 0,
				Count
			};

			namespace DownSample6x6 {
				enum {
					RC_Consts = 0,
					SI_InputMap,
					UO_OutputMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				CP_DownSample2x2 = 0,
				CP_DownSample4x4,
				CP_DownSample6x6,
				Count
			};
		}

		class TextureScalerClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
			};

		public:
			TextureScalerClass();
			virtual ~TextureScalerClass();

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
			BOOL DownSample2Nx2N(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pInputMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
				Foundation::Resource::GpuResource* const pOutputMap,
				D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
				UINT srcTexDimX, UINT srcTexDimY, UINT dstTexDimX, UINT dstTexDimY,
				UINT kernelRadius);

		private:
			InitData mInitData{};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures{};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};
		};

		using InitDataPtr = std::unique_ptr<TextureScalerClass::InitData>;

		InitDataPtr MakeInitData();
	}
}