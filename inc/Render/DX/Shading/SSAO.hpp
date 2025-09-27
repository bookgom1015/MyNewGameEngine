#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace SSAO {
		namespace Shader {
			enum Type {
				CS_SSAO = 0,
				CS_AccumulateAO,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_DrawAO = 0,
				GR_DenoiseAO,
				Count
			};

			namespace DrawAO {
				enum {
					CB_AO = 0,
					RC_Consts,
					SI_NormalDepthMap,
					SI_PositionMap,
					SI_RandomVectorMap,
					UO_AOMap,
					UO_DebugMap,
					Count
				};
			}

			namespace DenoiseAO {
				enum {
					CB_AO = 0,
					RC_Consts,
					SI_NormalDepthMap,
					SI_ReprojNormalDepthMap,
					SI_CachedNormalDepthMap,
					SI_VelocityMap,
					SI_InputAOMap,
					UIO_OutputAOMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum {
				CS_DrawAO = 0,
				CS_DenoiseAO,
				Count
			};
		}

		class SSAOClass : public Foundation::ShadingObject {
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
			SSAOClass();
			virtual ~SSAOClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* AOMap() const;
			__forceinline Foundation::Resource::GpuResource* AOMap(UINT index) const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE AOMapSrv() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE AOMapSrv(UINT index) const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE AOMapUav(UINT index) const;

			__forceinline void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]);

			__forceinline constexpr UINT TexWidth() const;
			__forceinline constexpr UINT TexHeight() const;

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
			BOOL BuildRandomVectorTexture();

			BOOL Run(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pCurrNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_currNormalDepthMap,
				Foundation::Resource::GpuResource* const pReprojNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_reprojNormalDepthMap,
				Foundation::Resource::GpuResource* const pCachedNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_CachedNormalDepthMap,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				Foundation::Resource::GpuResource* const pVelocityMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap);

		private:
			BOOL BuildRandomVectorMapResource();
			BOOL BuildRandomVectorMapDescriptor();

			BOOL BuildResources();
			BOOL BuildDescriptors();

			void BuildOffsetVecotrs();

			BOOL Draw(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pCurrNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_currNormalDepthMap,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap);
			BOOL Accumulate(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pCurrNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_currNormalDepthMap,
				Foundation::Resource::GpuResource* const pReprojNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_reprojNormalDepthMap,
				Foundation::Resource::GpuResource* const pCachedNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_CachedNormalDepthMap,
				Foundation::Resource::GpuResource* const pVelocityMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap);

		private:
			InitData mInitData;

			D3D12_VIEWPORT mViewport;
			D3D12_RECT mScissorRect;

			UINT mTexWidth = 0;
			UINT mTexHeight = 0;

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array< Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::unique_ptr<Foundation::Resource::GpuResource> mRandomVectorMap;
			std::unique_ptr<Foundation::Resource::GpuResource> mRandomVectorMapUploadBuffer;
			D3D12_CPU_DESCRIPTOR_HANDLE mhRandomVectorMapCpuSrv;
			D3D12_GPU_DESCRIPTOR_HANDLE mhRandomVectorMapGpuSrv;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, 2> mAOMaps;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> mhAOMapCpuSrvs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2> mhAOMapGpuSrvs;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> mhAOMapCpuUavs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2> mhAOMapGpuUavs;

			std::unique_ptr<Foundation::Resource::GpuResource> mDebugMap;
			D3D12_CPU_DESCRIPTOR_HANDLE mhDebugMapCpuUav;
			D3D12_GPU_DESCRIPTOR_HANDLE mhDebugMapGpuUav;

			DirectX::XMFLOAT4 mOffsets[14];

			UINT mCurrentAOMapIndex = 0;
		};

		using InitDataPtr = std::unique_ptr<SSAOClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "SSAO.inl"