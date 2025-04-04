#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX {
	namespace Foundation {
		namespace Core {
			class Device;
			class CommandObject;
		}

		namespace Resource {
			class GpuResource;
		}
	}

	namespace Shading {
		namespace Util {
			class ShaderManager;
		}

		namespace EnvironmentMap {
			namespace Shader {
				enum Type {
					E_VS_DrawSkySphere = 0,
					E_PS_DrawSkySphere,
					Count
				};
			}

			namespace RootSignature {
				enum Type {
					E_DrawSkySphere = 0,
					Count
				};

				namespace DrawSkySphere {
					enum {
						ECB_Pass = 0,
						ECB_Object,
						ESI_EnvCubeMap,
						Count
					};
				}
			}

			namespace PipelineState {
				enum Type {
					EG_DrawSkySphere = 0,
					Count
				};
			}

			class EnvironmentMapClass : public Render::DX::Foundation::ShadingObject {
			public:
				struct InitData {
					Foundation::Core::Device* Device = nullptr;
					Foundation::Core::CommandObject* CommandObject = nullptr;
					Util::ShaderManager* ShaderManager = nullptr;
				};

			public:
				EnvironmentMapClass();
				virtual ~EnvironmentMapClass() = default;

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

			public:
				BOOL SetEnvironmentMap(LPCWSTR filePath);

				BOOL GenerateMipmap();

			private:
				BOOL BuildDescriptors();

			private:
				InitData mInitData;

				std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

				std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
				std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

				std::unique_ptr<Foundation::Resource::GpuResource> mEquirectangularMap;
				CD3DX12_CPU_DESCRIPTOR_HANDLE mhEquirectangularMapCpuSrv;
				CD3DX12_GPU_DESCRIPTOR_HANDLE mhEquirectangularMapGpuSrv;
			};

			using InitDataPtr = std::unique_ptr<EnvironmentMapClass::InitData>;

			InitDataPtr MakeInitData();
		}
	}
}