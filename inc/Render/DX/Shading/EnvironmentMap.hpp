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
				VS_ConvoluteDiffuseIrradiance,
				GS_ConvoluteDiffuseIrradiance,
				PS_ConvoluteDiffuseIrradiance,
				VS_ConvoluteSpecularIrradiance,
				GS_ConvoluteSpecularIrradiance,
				PS_ConvoluteSpecularIrradiance,
				VS_IntegrateBrdf,
				MS_IntegrateBrdf,
				PS_IntegrateBrdf,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_DrawSkySphere = 0,
				GR_ConvoluteDiffuseIrradiance,
				GR_ConvoluteSpecularIrradiance,
				GR_IntegrateBrdf,
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

			namespace ConvoluteDiffuseIrradiance {
				enum {
					CB_ProjectToCube = 0,
					SI_CubeMap,
					Count
				};
			}

			namespace ConvoluteSpecularIrradiance {
				enum {
					CB_ProjectToCube = 0,
					RC_Consts,
					SI_EnvCubeMap,
					Count
				};
			}

			namespace IntegrateBrdf {
				enum {
					CB_Pass = 0,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_DrawSkySphere = 0,
				MP_DrawSkySphere,
				GP_ConvoluteDiffuseIrradiance,
				GP_ConvoluteSpecularIrradiance,
				GP_IntegrateBrdf,
				MP_IntegrateBrdf,
				Count
			};
		}

		class EnvironmentMapClass : public Render::DX::Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL MeshShaderSupported{};
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
			};

			enum TaskType {
				E_None = 0,
				E_NeedToSaveEnvCubeMap				 = 1 << 0,
				E_NeedToSaveDiffuseIrradianceCubeMap = 1 << 1,
				E_NeedToSavePrefilteredEnvCubeMap	 = 1 << 2,
				E_NeedToSaveBrdfLutMap				 = 1 << 3,
				E_NeedToGenEnvCubeMap				 = 1 << 4,
				E_NeedToGenDiffuseIrradianceCubeMap	 = 1 << 5,
				E_NeedToGenPrefilteredEnvCubeMap	 = 1 << 6,
				E_NeedToGenBrdfLutMap				 = 1 << 7
			};

		public:
			EnvironmentMapClass();
			virtual ~EnvironmentMapClass();

		public:
			__forceinline Foundation::Resource::GpuResource* DiffuseIrradianceCubeMap() const;
			__forceinline D3D12_GPU_DESCRIPTOR_HANDLE DiffuseIrradianceCubeMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* PrefilteredEnvironmentCubeMap() const;
			__forceinline D3D12_GPU_DESCRIPTOR_HANDLE PrefilteredEnvironmentCubeMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* BrdfLutMap() const;
			__forceinline D3D12_GPU_DESCRIPTOR_HANDLE BrdfLutMapSrv() const;

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

		public:
			BOOL SetEnvironmentMap(
				Foundation::Resource::FrameResource* const pFrameResource,
				Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator, 
				Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
				LPCWSTR fileName, LPCWSTR baseDir);

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
			BOOL Load(LPCWSTR fileName, LPCWSTR baseDir);
			BOOL Generate(
				Foundation::Resource::FrameResource* const pFrameResource,
				Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator,
				Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
				LPCWSTR fileName, LPCWSTR baseDir);
			BOOL Save(LPCWSTR fileName, LPCWSTR baseDir);

			BOOL CreateEquirectangularMap();
			BOOL BuildEquirectangularMapDescriptors();

			BOOL CreateEnvironmentCubeMap();
			BOOL BuildEnvironmentMapDescriptors(BOOL bNeedRtv);

			BOOL CreateDiffuseIrradianceCubeMap();
			BOOL BuildDiffuseIrradianceCubeMapDescriptors(BOOL bNeedRtv);

			BOOL CreatePrefilteredEnvironmentCubeMap();
			BOOL BuildPrefilteredEnvironmentCubeMapDescriptors(BOOL bNeedRtv);

			BOOL CreateBrdfLutMap();
			BOOL BuildBrdfLutMapDescriptors(BOOL bNeedRtv);

			BOOL GenerateMipmap(Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator);
			BOOL ConvertEquirectangularMapToCubeMap(
				Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
				D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube);

			BOOL DrawDiffuseIrradianceCubeMap(D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube);
			BOOL DrawPrefilteredEnvironmentCubeMap(D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube);
			BOOL DrawBrdfLutMap(D3D12_GPU_VIRTUAL_ADDRESS cbPass);

		private:
			InitData mInitData{};

			TaskType mTasks{};

			D3D12_VIEWPORT mViewport{};
			D3D12_RECT mScissorRect{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures{};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates{};

			std::unique_ptr<Foundation::Resource::GpuResource> mTemporaryEquirectangularMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhTemporaryEquirectangularMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhTemporaryEquirectangularMapGpuSrv{};

			std::unique_ptr<Foundation::Resource::GpuResource> mEquirectangularMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhEquirectangularMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhEquirectangularMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhEquirectangularMapCpuRtvs[ShadingConvention::MipmapGenerator::MaxMipLevel]{};

			std::unique_ptr<Foundation::Resource::GpuResource> mEnvironmentCubeMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhEnvironmentCubeMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhEnvironmentCubeMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhEnvironmentCubeMapCpuRtvs[ShadingConvention::MipmapGenerator::MaxMipLevel]{};

			std::unique_ptr<Foundation::Resource::GpuResource> mDiffuseIrradianceCubeMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhDiffuseIrradianceCubeMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhDiffuseIrradianceCubeMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhDiffuseIrradianceCubeMapCpuRtv{};

			std::unique_ptr<Foundation::Resource::GpuResource> mPrefilteredEnvironmentCubeMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhPrefilteredEnvironmentCubeMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhPrefilteredEnvironmentCubeMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhPrefilteredEnvironmentCubeMapCpuRtvs[ShadingConvention::MipmapGenerator::MaxMipLevel]{};

			std::unique_ptr<Foundation::Resource::GpuResource> mBrdfLutMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhBrdfLutMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhBrdfLutMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhBrdfLutMapCpuRtv{};
		};

		using InitDataPtr = std::unique_ptr<EnvironmentMapClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "EnvironmentMap.inl"