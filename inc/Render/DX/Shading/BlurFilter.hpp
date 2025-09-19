#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace BlurFilter {
		__forceinline INT CalcDiameter(FLOAT sigma);
		__forceinline BOOL CalcGaussWeights(FLOAT sigma, FLOAT weights[]);

		namespace Shader {
			enum Type {
				CS_GaussianBlurFilter3x3CS = 0,
				VS_GaussianBlurFilter3x3,
				PS_GaussianBlurFilter3x3,
				CS_GaussianBlurFilterRG3x3CS,
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
				GP_GaussianBlurFilter3x3,
				CP_GaussianBlurFilterRG3x3,
				Count
			};
		}

		class BlurFilterClass : public Foundation::ShadingObject {
		public:
			struct InitData {
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
				PipelineState::Type type,
				Foundation::Resource::GpuResource* const pInputMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
				Foundation::Resource::GpuResource* const pOutputMap,
				D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
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