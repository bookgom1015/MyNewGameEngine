#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace TAA {
		namespace Shader {
			enum Type {
				VS_TAA = 0,
				MS_TAA,
				PS_TAA,
				Count
			};
		}

		namespace RootSignature {
			namespace Default {
				enum {
					RC_Consts = 0,
					SI_BackBuffer,
					SI_HistoryMap,
					SI_VelocityMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_TAA = 0,
				MP_TAA,
				Count
			};
		}

		class TAAClass : public Foundation::ShadingObject {
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
			TAAClass();
			virtual ~TAAClass() = default;

		public:
			__forceinline constexpr UINT HaltonSequenceSize() const;
			__forceinline constexpr DirectX::XMFLOAT2 HaltonSequence(UINT index) const;

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
			BOOL ApplyTAA(
				Foundation::Resource::FrameResource* const pFrameResource,
				const D3D12_VIEWPORT& viewport,
				const D3D12_RECT& scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const pBackBufferCopy,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
				Foundation::Resource::GpuResource* const pVelocityMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap,
				FLOAT factor);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::unique_ptr<Foundation::Resource::GpuResource> mHistoryMap;
			D3D12_CPU_DESCRIPTOR_HANDLE mhHistoryMapCpuSrv;
			D3D12_GPU_DESCRIPTOR_HANDLE mhHistoryMapGpuSrv;

			const std::array<DirectX::XMFLOAT2, 16> mHaltonSequence = {
				DirectX::XMFLOAT2(0.5f, 0.333333f),
				DirectX::XMFLOAT2(0.25f, 0.666667f),
				DirectX::XMFLOAT2(0.75f, 0.111111f),
				DirectX::XMFLOAT2(0.125f, 0.444444f),
				DirectX::XMFLOAT2(0.625f, 0.777778f),
				DirectX::XMFLOAT2(0.375f, 0.222222f),
				DirectX::XMFLOAT2(0.875f, 0.555556f),
				DirectX::XMFLOAT2(0.0625f, 0.888889f),
				DirectX::XMFLOAT2(0.5625f, 0.037037f),
				DirectX::XMFLOAT2(0.3125f, 0.37037f),
				DirectX::XMFLOAT2(0.8125f, 0.703704f),
				DirectX::XMFLOAT2(0.1875f, 0.148148f),
				DirectX::XMFLOAT2(0.6875f, 0.481481f),
				DirectX::XMFLOAT2(0.4375f, 0.814815f),
				DirectX::XMFLOAT2(0.9375f, 0.259259f),
				DirectX::XMFLOAT2(0.03125f, 0.592593f)
			};
			std::array<DirectX::XMFLOAT2, 16> mFittedToBakcBufferHaltonSequence;
		};

		using InitDataPtr = std::unique_ptr<TAAClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "TAA.inl"