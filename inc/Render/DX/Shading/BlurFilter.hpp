#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace BlurFilter {
		__forceinline INT CalcDiameter(FLOAT sigma);
		__forceinline BOOL CalcGaussWeights(FLOAT sigma, FLOAT weights[]);

		namespace Shader {
			enum Type {
				CS_GaussianBlurFilter3x3CS = 0,
				CS_GaussianBlurFilterRG3x3CS,
				CS_GaussianBlurFilterNxNCS_3x3,
				CS_GaussianBlurFilterNxNCS_5x5,
				CS_GaussianBlurFilterNxNCS_7x7,
				CS_GaussianBlurFilterNxNCS_9x9,
				VS_GaussianBlurFilterRGBANxN_3x3,
				MS_GaussianBlurFilterRGBANxN_3x3,
				PS_GaussianBlurFilterRGBANxN_3x3,
				VS_GaussianBlurFilterRGBANxN_5x5,
				MS_GaussianBlurFilterRGBANxN_5x5,
				PS_GaussianBlurFilterRGBANxN_5x5,
				VS_GaussianBlurFilterRGBANxN_7x7,
				MS_GaussianBlurFilterRGBANxN_7x7,
				PS_GaussianBlurFilterRGBANxN_7x7,
				VS_GaussianBlurFilterRGBANxN_9x9,
				MS_GaussianBlurFilterRGBANxN_9x9,
				PS_GaussianBlurFilterRGBANxN_9x9,
				Count
			};
		}

		namespace RootSignature {
			namespace Default {
				enum {
					RC_Consts = 0,
					SI_InputMap,
					UO_OutputMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				CP_GaussianBlurFilter3x3 = 0,
				CP_GaussianBlurFilterRG3x3,
				CP_GaussianBlurFilterNxN_3x3,
				CP_GaussianBlurFilterNxN_5x5,
				CP_GaussianBlurFilterNxN_7x7,
				CP_GaussianBlurFilterNxN_9x9,
				GP_GaussianBlurFilterNxN_3x3,
				MP_GaussianBlurFilterNxN_3x3,
				GP_GaussianBlurFilterNxN_5x5,
				MP_GaussianBlurFilterNxN_5x5,
				GP_GaussianBlurFilterNxN_7x7,
				MP_GaussianBlurFilterNxN_7x7,
				GP_GaussianBlurFilterNxN_9x9,
				MP_GaussianBlurFilterNxN_9x9,
				Count
			};
		}

		namespace BlurType {
			enum Compute {
				C_GaussianBlurFilter3x3,
				C_GaussianBlurFilterRG3x3,
				C_GaussianBlurFilterNxN_3x3,
				C_GaussianBlurFilterNxN_5x5,
				C_GaussianBlurFilterNxN_7x7,
				C_GaussianBlurFilterNxN_9x9,
				
			};

			enum Graphics {
				G_GaussianBlurFilterNxN_3x3,
				G_GaussianBlurFilterNxN_5x5,
				G_GaussianBlurFilterNxN_7x7,
				G_GaussianBlurFilterNxN_9x9
			};
		}

		class BlurFilterClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL MeshShaderSupported = FALSE;
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
			};

		public:
			BlurFilterClass();
			virtual ~BlurFilterClass() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) override;
			virtual BOOL BuildPipelineStates() override;

		public:
			BOOL GaussianBlur(
				Foundation::Resource::FrameResource* const pFrameResource,
				BlurType::Compute type,
				Foundation::Resource::GpuResource* const pInputMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
				Foundation::Resource::GpuResource* const pOutputMap,
				D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
				UINT texWidth, UINT texHeight);

			BOOL GaussianBlur(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_VIEWPORT viewport,
				D3D12_RECT scissorRect,
				BlurType::Graphics type,
				Foundation::Resource::GpuResource* const pInputMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
				Foundation::Resource::GpuResource* const pOutputMap,
				D3D12_CPU_DESCRIPTOR_HANDLE ro_outputMap,
				UINT texWidth, UINT texHeight);

		private:
			InitData mInitData;

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;
		};

		using InitDataPtr = std::unique_ptr<BlurFilterClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "BlurFilter.inl"