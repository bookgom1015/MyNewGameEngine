#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

#include <functional>

namespace Render::DX::Shading {
	namespace Bloom {
		namespace Shader {
			enum Type {
				VS_ExtractHighlights = 0,
				MS_ExtractHighlights,
				PS_ExtractHighlights,
				VS_ApplyBloom,
				MS_ApplyBloom,
				PS_ApplyBloom,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_ExtractHighlights = 0,
				GR_ApplyBloom,
				Count
			};

			namespace ExtractHighlights {
				enum {
					RC_Consts = 0,
					SI_BackBuffer,
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
				GP_ExtractHighlights = 0,
				MP_ExtractHighlights,
				GP_ApplyBloom,
				MP_ApplyBloom,
				Count
			};
		}

		namespace Resource {
			enum Type {
				E_FullRes = 0,
				E_QuaterRes,
				E_EighteenthRes,
				Count
			};
		}

		class BloomClass : public Foundation::ShadingObject {
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
			BloomClass();
			virtual ~BloomClass() = default;

		public:
			__forceinline constexpr UINT TextureWidth() const;
			__forceinline constexpr UINT TextureHeight() const;

			__forceinline Foundation::Resource::GpuResource* BloomMap() const;
			__forceinline D3D12_GPU_DESCRIPTOR_HANDLE BloomMapSrv() const;

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
			virtual BOOL OnResize(UINT width, UINT height) override;

		public:
			BOOL ExtractHighlights(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer,
				FLOAT threshold, FLOAT softknee);
			BOOL BlurHighlights(
				std::function<BOOL(
					D3D12_VIEWPORT viewport,
					D3D12_RECT scissorRect,
					Foundation::Resource::GpuResource* const,
					D3D12_GPU_DESCRIPTOR_HANDLE,
					Foundation::Resource::GpuResource* const,
					D3D12_CPU_DESCRIPTOR_HANDLE,
					UINT, UINT)> func);
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

		private:
			InitData mInitData;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::Count> mHighlightMaps;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Resource::Count> mhHighlightMapCpuSrvs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Resource::Count> mhHighlightMapGpuSrvs;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Resource::Count> mhHighlightMapCpuRtvs;
		};

		using InitDataPtr = std::unique_ptr<BloomClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

constexpr UINT Render::DX::Shading::Bloom::BloomClass::TextureWidth() const {
	return mInitData.ClientWidth >> 2;
}

constexpr UINT Render::DX::Shading::Bloom::BloomClass::TextureHeight() const {
	return mInitData.ClientHeight >> 2;
}

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::Bloom::BloomClass::BloomMap() const {
	return mHighlightMaps[Resource::E_EighteenthRes].get();
}
D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::Bloom::BloomClass::BloomMapSrv() const {
	return mhHighlightMapGpuSrvs[Resource::E_EighteenthRes];
}