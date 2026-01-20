#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace SSAO {
		namespace Shader {
			enum Type {
				CS_SSAO = 0,
				Count
			};
		}

		namespace RootSignature {
			namespace Default {
				enum {
					CB_AO = 0,
					RC_Consts,
					SI_NormalDepthMap,
					SI_PositionMap,
					SI_RandomVectorMap,
					UO_AOCoefficientMap,
					UO_RayHitDistMap,
					UO_DebugMap,
					Count
				};
			}
		}

		namespace Resource {
			namespace AO {
				enum Type {
					E_AOCoefficient = 0,
					E_RayHitDistance,
					Count
				};
			}

			namespace TemporalCache {
				enum Type {
					E_TSPP = 0,
					E_RayHitDistance,
					E_AOCoefficientSquaredMean,
					Count
				};
			}
		}

		namespace Descriptor {
			namespace AO {
				enum Type {
					ES_AOCoefficient = 0,
					EU_AOCoefficient,
					ES_RayHitDistance,
					EU_RayHitDistance,
					Count
				};
			}

			namespace TemporalCache {
				enum Type {
					ES_TSPP = 0,
					EU_TSPP,
					ES_RayHitDistance,
					EU_RayHitDistance,
					ES_AOCoefficientSquaredMean,
					EU_AOCoefficientSquaredMean,
					Count
				};
			}

			namespace TemporalAO {
				enum Type {
					E_Srv = 0,
					E_Uav,
					Count
				};
			}
		}

		class SSAOClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
				UINT ClientWidth{};
				UINT ClientHeight{};
			};

		public:
			SSAOClass();
			virtual ~SSAOClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* AOCoefficientResource(Resource::AO::Type type) const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE AOCoefficientDescriptor(Descriptor::AO::Type type) const;

			__forceinline Foundation::Resource::GpuResource* TemporalAOCoefficientResource(UINT frame) const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE TemporalAOCoefficientSrv(UINT frame) const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE TemporalAOCoefficientUav(UINT frame) const;

			__forceinline Foundation::Resource::GpuResource* TemporalCacheResource(Resource::TemporalCache::Type type, UINT frame) const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE TemporalCacheDescriptor(Descriptor::TemporalCache::Type type, UINT frame) const;

			__forceinline constexpr UINT CurrentTemporalCacheFrameIndex() const;
			__forceinline constexpr UINT CurrentTemporalAOFrameIndex() const;

			__forceinline void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]);

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
			BOOL BuildRandomVectorTexture();

			BOOL DrawAO(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pCurrNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_currNormalDepthMap,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap);

			UINT MoveToNextTemporalCacheFrame();
			UINT MoveToNextTemporalAOFrame();

		private:
			BOOL BuildRandomVectorMapResource();
			BOOL BuildRandomVectorMapDescriptor();

			BOOL BuildResources();
			BOOL BuildDescriptors();

			void BuildOffsetVecotrs();

		private:
			InitData mInitData{};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature{};
			Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			std::unique_ptr<Foundation::Resource::GpuResource> mRandomVectorMap{};
			std::unique_ptr<Foundation::Resource::GpuResource> mRandomVectorMapUploadBuffer{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhRandomVectorMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhRandomVectorMapGpuSrv{};

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::AO::Count> mAOResources{};
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Descriptor::AO::Count> mhAOResourceCpus{};
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Descriptor::AO::Count> mhAOResourceGpus{};

			std::array<std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::TemporalCache::Count>, 2> mTemporalCaches{};
			std::array<std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Descriptor::TemporalCache::Count>, 2> mhTemporalCacheCpus{};
			std::array<std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Descriptor::TemporalCache::Count>, 2> mhTemporalCacheGpus{};

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, 2> mTemporalAOResources{};
			std::array<std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Descriptor::TemporalAO::Count>, 2> mhTemporalAOResourceCpus{};
			std::array<std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Descriptor::TemporalAO::Count>, 2> mhTemporalAOResourceGpus{};

			std::unique_ptr<Foundation::Resource::GpuResource> mDebugMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhDebugMapCpuUav{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhDebugMapGpuUav{};

			DirectX::XMFLOAT4 mOffsets[14]{};

			UINT mCurrentTemporalCacheFrameIndex{};
			UINT mCurrentTemporalAOFrameIndex{};
		};

		using InitDataPtr = std::unique_ptr<SSAOClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "SSAO.inl"