#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading::Util {
	namespace MipmapGenerator {
		namespace Shader {
			enum Type {
				E_VS_GenerateMipmap = 0,
				E_PS_GenerateMipmap,
				E_PS_CopyMap,
				Count
			};
		}

		namespace RootSignature {
			namespace Default {
				enum {
					EC_Consts = 0,
					ESI_InputMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				EG_GenerateMipmap = 0,
				EG_CopyMap,
				Count
			};
		}

		class MipmapGeneratorClass : public Render::DX::Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
			};

		public:
			MipmapGeneratorClass() = default;
			virtual ~MipmapGeneratorClass() = default;

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
			BOOL GenerateMipmap(
				Foundation::Resource::GpuResource* const pOutput,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_outputs[],
				Foundation::Resource::GpuResource* const pInput,
				D3D12_GPU_DESCRIPTOR_HANDLE si_input,
				UINT maxMipLevel, UINT width, UINT height);

		private:
			BOOL CopyMap(
				Foundation::Resource::GpuResource* const pOutput,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_output,
				Foundation::Resource::GpuResource* const pInput,
				D3D12_GPU_DESCRIPTOR_HANDLE si_input,
				UINT width, UINT height);
			BOOL GenerateMipmap(
				Foundation::Resource::GpuResource* const pOutput,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_outputs[],
				D3D12_GPU_DESCRIPTOR_HANDLE si_input,
				UINT maxMipLevel, UINT width, UINT height);

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates = {};
		};

		using InitDataPtr = std::unique_ptr<MipmapGeneratorClass::InitData>;

		InitDataPtr MakeInitData();
	}
}