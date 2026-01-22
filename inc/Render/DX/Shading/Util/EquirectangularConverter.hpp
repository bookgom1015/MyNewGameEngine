#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading::Util {
	namespace EquirectangularConverter {
		namespace Shader {
			enum Type {
				VS_ConvCubeToEquirect = 0,
				PS_ConvCubeToEquirect,
				VS_ConvEquirectToCube,
				GS_ConvEquirectToCube,
				PS_ConvEquirectToCube,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_ConvEquirectToCube = 0,
				GR_ConvCubeToEquirect,
				Count
			};

			namespace ConvEquirectToCube {
				enum {
					CB_ProjectToCube = 0,
					SI_EquirectangularMap,
					Count
				};
			}

			namespace ConvCubeToEquirect {
				enum {
					RC_Consts = 0,
					SI_CubeMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_ConvEquirectToCube = 0,
				GP_ConvCubeToEquirect,
				Count
			};
		}

		class EquirectangularConverterClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
			};

		public:
			EquirectangularConverterClass();
			virtual ~EquirectangularConverterClass();

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
			BOOL ConvertEquirectangularToCube(
				Foundation::Resource::GpuResource* const pCube,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_cubes[],
				Foundation::Resource::GpuResource* const pEquirect,
				D3D12_GPU_DESCRIPTOR_HANDLE si_equirect,
				D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube,
				UINT width, UINT height,
				UINT maxMipLevel = 0);
			BOOL ConvertEquirectangularToCube(
				Foundation::Resource::GpuResource* const pCube,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_cube,
				Foundation::Resource::GpuResource* const pEquirect,
				D3D12_GPU_DESCRIPTOR_HANDLE si_equirect,
				D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube,
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
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures{};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates{};
		};

		using InitDataPtr = std::unique_ptr<EquirectangularConverterClass::InitData>;

		InitDataPtr MakeInitData();
	}
}