#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace Bloom {
		namespace Shader {
			enum Type {
				CS_ExtractHighlights = 0,
				CS_BlendBloomWithDownSampled,
				VS_ApplyBloom,
				MS_ApplyBloom,
				PS_ApplyBloom,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_ExtractHighlights = 0,
				GR_BlendBloomWithDownSampled,
				GR_ApplyBloom,
				Count
			};

			namespace ExtractHighlights {
				enum {
					RC_Consts = 0,
					UIO_HighlightMap,
					Count
				};
			}

			namespace BlendBloomWithDownSampled {
				enum {
					RC_Consts = 0,
					SI_LowerScaleMap,
					UIO_HigherScaleMap,
					Count
				};
			}

			namespace ApplyBloom {
				enum {
					SI_BackBuffer = 0,
					SI_BloomMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				CP_ExtractHighlights = 0,
				CP_BlendBloomWithDownSampled,
				GP_ApplyBloom,
				MP_ApplyBloom,
				Count
			};
		}

		namespace Resource {
			enum Type {
				E_4thRes = 0,
				E_16thRes,
				E_64thRes,
				E_256thRes,
				Count
			};
		}

		class BloomClass : public Foundation::ShadingObject {
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

			using DownSampleFunc = std::function<BOOL(
				Foundation::Resource::GpuResource* const,
				D3D12_GPU_DESCRIPTOR_HANDLE,
				Foundation::Resource::GpuResource* const,
				D3D12_GPU_DESCRIPTOR_HANDLE,
				UINT, UINT, UINT, UINT,
				UINT)>;

			using BlurFunc = std::function < BOOL(
				Foundation::Resource::GpuResource* const,
				D3D12_GPU_DESCRIPTOR_HANDLE,
				Foundation::Resource::GpuResource* const,
				D3D12_GPU_DESCRIPTOR_HANDLE,
				UINT, UINT)>;

		public:
			BloomClass();
			virtual ~BloomClass() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures() override;
			virtual BOOL BuildPipelineStates() override;
			virtual BOOL BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) override;
			virtual BOOL OnResize(UINT width, UINT height) override;

		public:
			BOOL ExtractHighlights(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer,
				FLOAT threshold, FLOAT softknee,
				DownSampleFunc downSampleFunc);
			BOOL BlurHighlights(
				Foundation::Resource::FrameResource* const pFrameResource,
				DownSampleFunc downSampleFunc, 
				BlurFunc blurFunc);
			BOOL ApplyBloom(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_VIEWPORT viewport,
				D3D12_RECT scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const pBackBufferCopy,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

			BOOL DownSampling(DownSampleFunc downSampleFunc);
			BOOL UpSamplingWithBlur(
				Foundation::Resource::FrameResource* const pFrameResource,
				BlurFunc blurFunc);

		private:
			InitData mInitData;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures{};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::Count> mHighlightMaps{};
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Resource::Count> mhHighlightMapCpuSrvs{};
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Resource::Count> mhHighlightMapGpuSrvs{};
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Resource::Count> mhHighlightMapCpuUavs{};
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Resource::Count> mhHighlightMapGpuUavs{};

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::Count> mBloomMaps{};
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Resource::Count> mhBloomMapCpuSrvs{};
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Resource::Count> mhBloomMapGpuSrvs{};
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Resource::Count> mhBloomMapCpuUavs{};
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Resource::Count> mhBloomMapGpuUavs{};
		};

		using InitDataPtr = std::unique_ptr<BloomClass::InitData>;

		InitDataPtr MakeInitData();
	}
}