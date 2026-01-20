#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Common::Render::ShadingArgument {
	struct ShadingArgumentSet;
}

namespace Render::DX::Shading {
	namespace RaySorting {
		namespace Shader {
			enum Type {
				CS_CountingSort = 0,
				Count
			};
		}

		namespace RootSignature {
			namespace Default {
				enum {
					CB_RaySorting = 0,
					SI_NormalDepthMap,
					UO_RayIndexOffsetMap,
					Count
				};
			}
		}

		class RaySortingClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Common::Render::ShadingArgument::ShadingArgumentSet* ShadingArgumentSet{};
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
				UINT ClientWidth{};
				UINT ClientHeight{};
			};

		public:
			RaySortingClass();
			virtual ~RaySortingClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* RayIndexOffsetMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE RayIndexOffsetMapSrv() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE RayIndexOffsetMapUav() const;

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
			BOOL CalcRayIndexOffset(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData{};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature{};
			Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			std::unique_ptr<Foundation::Resource::GpuResource> mRayIndexOffsetMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhRayIndexOffsetMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhRayIndexOffsetMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhRayIndexOffsetMapCpuUav{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhRayIndexOffsetMapGpuUav{};
		};

		using InitDataPtr = std::unique_ptr<RaySortingClass::InitData>;

		InitDataPtr MakeInitData();

	}
}

#include "RaySorting.inl"