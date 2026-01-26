#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"
#include "Render/DX/Foundation/Resource/StructuredBuffer.hpp"

namespace Common {
	namespace Foundation{
		class MultiJittered;
	}

	namespace Render {
		namespace ShadingArgument {
			struct ShadingArgumentSet;
		}
	}
}

namespace Render::DX::Shading {
	namespace RayGen {
		namespace Shader {
			enum Type {
				CS_RayGen = 0,
				Count
			};
		}

		namespace RootSignature {
			namespace Default {
				enum {
					CB_RayGen = 0,
					SB_SampleSets,
					SI_NormalDepthMap,
					SI_PositionMap,
					UO_RayDirectionOriginDepthMap,
					UO_DebugMap,
					Count
				};
			}
		}

		class RayGenClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Common::Render::ShadingArgument::ShadingArgumentSet* ShadingArgumentSet{};
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
				UINT ClientWidth{};
				UINT ClientHeight{};
				UINT MaxSamplesPerPixel{};
				UINT* SamplesPerPixel{};
				UINT MaxSampleSetDistributedAcrossPixels{};
				UINT* SampleSetDistributedAcrossPixels{};
				UINT* CurrentFrameIndex{};
			};
		public:
			RayGenClass();
			virtual ~RayGenClass();

		public:
			__forceinline Foundation::Resource::GpuResource* RayDirectionOriginDepthMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE RayDirectionOriginDepthMapSrv() const;

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
			virtual BOOL OnResize(UINT width, UINT height) override;
			virtual BOOL Update() override;

		public:
			UINT NumSampleSets() const;
			UINT NumSamples() const;
			UINT Seed();

		public:
			BOOL GenerateRays(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				BOOL bCheckboardRayGeneration);

		private:
			void BuildSamples();
			void UpdateStructuredBuffers();

			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData{};

			std::unique_ptr<Common::Foundation::MultiJittered> mRandomSampler{};
			std::mt19937 mGeneratorURNG{};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature{};
			Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Foundation::Resource::StructuredBuffer<ShadingConvention::RayGen::AlignedUnitSquareSample2D> mSamplesGPUBuffer{};
			Foundation::Resource::StructuredBuffer<ShadingConvention::RayGen::AlignedHemisphereSample3D> mHemisphereSamplesGPUBuffer{};

			std::unique_ptr<Foundation::Resource::GpuResource> mRayDirectionOriginDepthMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhRayDirectionOriginDepthMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhRayDirectionOriginDepthMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhRayDirectionOriginDepthMapCpuUav{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhRayDirectionOriginDepthMapGpuUav{};

			std::unique_ptr<Foundation::Resource::GpuResource> mDebugMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhDebugMapCpuUav{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhDebugMapGpuUav{};
		};

		using InitDataPtr = std::unique_ptr<RayGenClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "RayGen.inl"