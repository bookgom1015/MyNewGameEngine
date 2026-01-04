#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace EyeAdaption {
		namespace Shader {
			enum Type {
				CS_ClearHistogram = 0,
				CS_LuminanceHistogram,
				CS_PercentileExtract,
				CS_TemporalSmoothing,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_LuminanceHistogram = 0,
				GR_PercentileExtract,
				GR_TemporalSmoothing,
				Count
			};

			namespace LuminanceHistogram {
				enum {
					RC_Consts = 0,
					SI_BackBuffer,
					UO_HistogramBuffer,
					Count
				};
			}

			namespace PercentileExtract {
				enum {
					RC_Consts = 0,
					UI_HistogramBuffer,
					UO_AvgLogLuminance,
					Count
				};
			}

			namespace TemporalSmoothing {
				enum {
					RC_Consts = 0,
					UI_AvgLogLuminance,
					UO_SmoothedLum,
					UIO_PrevLum,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				CP_ClearHistogram = 0,
				CP_LuminanceHistogram,
				CP_PercentileExtract,
				CP_TemporalSmoothing,
				Count
			};
		}

		class EyeAdaptionClass : public Foundation::ShadingObject {
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
			EyeAdaptionClass();
			virtual ~EyeAdaptionClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* Luminance() const;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures() override;
			virtual BOOL BuildPipelineStates() override;

		public:
			BOOL ClearHistogram(
				Foundation::Resource::FrameResource* const pFrameResource);
			BOOL BuildLuminanceHistogram(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer);
			BOOL PercentileExtract(
				Foundation::Resource::FrameResource* const pFrameResource);
			BOOL TemporalSmoothing(
				Foundation::Resource::FrameResource* const pFrameResource,
				FLOAT deltaTime);

		private:
			BOOL BuildResources();

		private:
			InitData mInitData;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, 
				RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 
				PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::unique_ptr<Foundation::Resource::GpuResource> mHistogramBuffer;
			std::unique_ptr<Foundation::Resource::GpuResource> mAvgLogLuminance;
			std::unique_ptr<Foundation::Resource::GpuResource> mPrevLuminance;
			std::unique_ptr<Foundation::Resource::GpuResource> mSmoothedLuminance;
		};

		using InitDataPtr = std::unique_ptr<EyeAdaptionClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "EyeAdaption.inl"