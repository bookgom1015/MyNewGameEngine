#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace Util {
		namespace MipmapGenerator {
			class MipmapGeneratorClass;
		}

		namespace EquirectangularConverter {
			class EquirectangularConverterClass;
		}
	}

	namespace EnvironmentMap {
		namespace Shader {
			enum Type {
				VS_DrawSkySphere = 0,
				MS_DrawSkySphere,
				PS_DrawSkySphere,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_DrawSkySphere = 0,
				Count
			};

			namespace DrawSkySphere {
				enum {
					CB_Pass = 0,
					CB_Object,
					RC_Consts,
					SI_VertexBuffer,
					SI_IndexBuffer,
					SI_EnvCubeMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_DrawSkySphere = 0,
				MP_DrawSkySphere,
				Count
			};
		}

		class EnvironmentMapClass : public Render::DX::Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL MeshShaderSupported = FALSE;
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
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
			BOOL SetEnvironmentMap(
				Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator, 
				Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
				D3D12_GPU_VIRTUAL_ADDRESS cbEquirectConv,
				LPCWSTR filePath);

			BOOL DrawSkySphere(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_VIEWPORT viewport,
				D3D12_RECT scissorRect,
				Foundation::Resource::GpuResource* const backBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const depthBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE dio_depthStencil,
				Foundation::RenderItem* const sphere);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

			BOOL CreateTemporaryEquirectangularMap(LPCWSTR filePath);
			BOOL CreateEquirectangularMap();
			BOOL BuildEquirectangularMapDescriptors();
			BOOL GenerateMipmap(Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator);
			BOOL ConvertEquirectangularMapToCubeMap(
				Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
				D3D12_GPU_VIRTUAL_ADDRESS cbEquirectConv);

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures = {};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates = {};

			std::unique_ptr<Foundation::Resource::GpuResource> mTemporaryEquirectangularMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhTemporaryEquirectangularMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhTemporaryEquirectangularMapGpuSrv;

			std::unique_ptr<Foundation::Resource::GpuResource> mEquirectangularMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhEquirectangularMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhEquirectangularMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhEquirectangularMapCpuRtvs[ShadingConvention::MipmapGenerator::MaxMipLevel] = {};

			std::unique_ptr<Foundation::Resource::GpuResource> mEnvironmentCubeMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhEnvironmentCubeMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhEnvironmentCubeMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhEnvironmentCubeMapCpuRtvs[ShadingConvention::MipmapGenerator::MaxMipLevel] = {};
		};

		using InitDataPtr = std::unique_ptr<EnvironmentMapClass::InitData>;

		InitDataPtr MakeInitData();
	}
}