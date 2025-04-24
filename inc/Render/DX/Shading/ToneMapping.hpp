#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace ToneMapping {
		namespace Shader {
			enum Type {
				VS_ToneMapping = 0,
				MS_ToneMapping,
				PS_ToneMapping,
				Count
			};
		}

		namespace RootSignature {
			namespace Default {
				enum {
					RC_Cosnts = 0,
					SI_Intermediate,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_ToneMapping = 0,
				MP_ToneMapping,
				Count
			};
		}

		class ToneMappingClass : public Foundation::ShadingObject {
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
			ToneMappingClass();
			virtual ~ToneMappingClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* InterMediateMapResource();
			__forceinline constexpr CD3DX12_GPU_DESCRIPTOR_HANDLE InterMediateMapSrv() const;
			__forceinline constexpr CD3DX12_CPU_DESCRIPTOR_HANDLE InterMediateMapRtv() const;

			__forceinline Foundation::Resource::GpuResource* InterMediateCopyMapResource();
			__forceinline constexpr CD3DX12_GPU_DESCRIPTOR_HANDLE InterMediateCopyMapSrv() const;

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
			BOOL Resolve(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D12_VIEWPORT& viewport,
				const D3D12_RECT& scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				FLOAT exposure);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::unique_ptr<Foundation::Resource::GpuResource> mIntermediateMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhIntermediateMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhIntermediateMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhIntermediateMapCpuRtv;

			std::unique_ptr<Foundation::Resource::GpuResource> mIntermediateCopyMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhIntermediateCopyMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhIntermediateCopyMapGpuSrv;
		};

		using InitDataPtr = std::unique_ptr<ToneMappingClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "ToneMapping.inl"