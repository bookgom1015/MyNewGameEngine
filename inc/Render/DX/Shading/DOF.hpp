#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace DOF {
		namespace Shader {
			enum Type {
				CS_CaclFocalDistance = 0,
				CS_CircleOfConfusion,
				VS_Bokeh,
				MS_Bokeh,
				PS_Bokeh,
				VS_BokehBlurNxN,
				MS_BokehBlurNxN,
				PS_BokehBlurNxN,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_CalcFocalDistance = 0,
				GR_CircleOfConfusion,
				GR_Bokeh,
				GR_BokehBlurNxN,
				Count
			};

			namespace CalcFocalDistance {
				enum {
					CB_Pass = 0,
					SI_PositionMap,
					UO_FocalDistanceBuffer,
					Count
				};
			}

			namespace CircleOfConfusion {
				enum {
					CB_Pass = 0,
					RC_Consts,
					SI_DepthMap,
					UI_FocalDistanceBuffer,
					UO_CircleOfConfusionMap,
					Count
				};
			}

			namespace Bokeh {
				enum {
					RC_Consts = 0,
					SI_BackBuffer,
					SI_CoCMap,
					Count
				};
			}

			namespace BokehBlurNxN {
				enum {
					RC_Consts = 0,
					SI_InputMap,
					SI_CoCMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				CP_CalcFocalDistance = 0,
				CP_CircleOfConfusion,
				GP_Bokeh,
				MP_Bokeh,
				GP_BokehBlurNxN,
				MP_BokehBlurNxN,
				Count
			};
		}

		class DOFClass : public Foundation::ShadingObject {
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
			DOFClass();
			virtual ~DOFClass() = default;

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
			BOOL CalcFocalDistance(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap);
			BOOL CircleOfConfusion(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap);
			BOOL Bokeh(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_VIEWPORT viewport,
				D3D12_RECT scissorRect,
				Foundation::Resource::GpuResource* const pBackBuffer,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const pBackBufferCopy,
				D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy);
			BOOL BokehBlur(
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

			BOOL BuildFixedResources();

		private:
			InitData mInitData;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::unique_ptr<Foundation::Resource::GpuResource> mFocalDistanceBuffer;

			std::unique_ptr<Foundation::Resource::GpuResource> mCircleOfConfusionMap;
			D3D12_CPU_DESCRIPTOR_HANDLE mhCircleOfConfusionMapCpuUav;
			D3D12_GPU_DESCRIPTOR_HANDLE mhCircleOfConfusionMapGpuUav;
			D3D12_CPU_DESCRIPTOR_HANDLE mhCircleOfConfusionMapCpuSrv;
			D3D12_GPU_DESCRIPTOR_HANDLE mhCircleOfConfusionMapGpuSrv;
		};

		using InitDataPtr = std::unique_ptr<DOFClass::InitData>;

		InitDataPtr MakeInitData();
	}
}
