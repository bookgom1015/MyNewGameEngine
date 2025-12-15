#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace DOF {
		namespace Shader {
			enum Type {
				CS_CaclFocalDistance = 0,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_CalcFocalDistance = 0,
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
		}

		namespace PipelineState {
			enum Type {
				CP_CalcFocalDistance = 0,
				Count
			};
		}

		class DOFClass : public Foundation::ShadingObject {
		public:
			struct InitData {
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

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

			BOOL BuildFixedResources();
			BOOL BuildFixedDescriptors();

		private:
			InitData mInitData;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::unique_ptr<Foundation::Resource::GpuResource> mFocalDistanceBuffer;
			D3D12_CPU_DESCRIPTOR_HANDLE mhFocalDistanceBufferCpuSrv;
			D3D12_GPU_DESCRIPTOR_HANDLE mhFocalDistanceBufferGpuSrv;
			D3D12_CPU_DESCRIPTOR_HANDLE mhFocalDistanceBufferCpuUav;
			D3D12_GPU_DESCRIPTOR_HANDLE mhFocalDistanceBufferGpuUav;
		};

		using InitDataPtr = std::unique_ptr<DOFClass::InitData>;

		InitDataPtr MakeInitData();
	}
}
