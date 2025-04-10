#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading::Util {
	namespace EquirectangularConverter {
		namespace Shader {
			enum Type {
				E_VS_ConvCubeToEquirect = 0,
				E_PS_ConvCubeToEquirect,
				E_VS_ConvEquirectToCube,
				E_GS_ConvEquirectToCube,
				E_PS_ConvEquirectToCube,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				E_ConvEquirectToCube = 0,
				E_ConvCubeToEquirect,
				Count
			};

			namespace ConvEquirectToCube {
				enum {
					ECB_EquirectConverter = 0,
					ESI_EquirectangularMap,
					Count
				};
			}

			namespace ConvCubeToEquirect {
				enum {
					EC_Consts = 0,
					ESI_CubeMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				EG_ConvEquirectToCube = 0,
				EG_ConvCubeToEquirect,
				Count
			};
		}

		class EquirectangularConverterClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
			};

		public:
			EquirectangularConverterClass() = default;
			virtual ~EquirectangularConverterClass() = default;

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
			BOOL ConvertEquirectangularToCube(
				Foundation::Resource::GpuResource* const pCube,
				CD3DX12_CPU_DESCRIPTOR_HANDLE ro_cubes[],
				Foundation::Resource::GpuResource* const pEquirect,
				D3D12_GPU_DESCRIPTOR_HANDLE si_equirect,
				D3D12_GPU_VIRTUAL_ADDRESS cbEquirectConv,
				UINT width, UINT height,
				UINT maxMipLevel = 0);
			BOOL ConvertEquirectangularToCube(
				Foundation::Resource::GpuResource* const pCube,
				CD3DX12_CPU_DESCRIPTOR_HANDLE ro_cube,
				Foundation::Resource::GpuResource* const pEquirect,
				D3D12_GPU_DESCRIPTOR_HANDLE si_equirect,
				D3D12_GPU_VIRTUAL_ADDRESS cbEquirectConv,
				D3D12_VIEWPORT viewport,
				D3D12_RECT scissorRect,
				UINT maxMipLevel = 0);
			BOOL ConvertCubeToEquirectangular(
				Foundation::Resource::GpuResource* const pEquirect,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_equirect,
				Foundation::Resource::GpuResource* const pCube,
				D3D12_GPU_DESCRIPTOR_HANDLE si_cube,
				D3D12_VIEWPORT viewport,
				D3D12_RECT scissorRect,
				UINT mipLevel = 0);

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures = {};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates = {};
		};

		using InitDataPtr = std::unique_ptr<EquirectangularConverterClass::InitData>;

		InitDataPtr MakeInitData();
	}
}