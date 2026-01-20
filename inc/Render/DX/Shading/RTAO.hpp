#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace RTAO {
		namespace Shader {
			enum Type {
				Lib_RTAO = 0,
				Count
			};
		}

		namespace RootSignature {
			enum {
				SI_AccelerationStructure = 0,
				CB_AO,
				SI_PositionMap,
				SI_NormalDepthMap,
				SI_RayDirectionOriginDepthMap,
				SI_RayIndexOffsetMap,
				UO_AOCoefficientMap,
				UO_RayHitDistanceMap,
				UO_DebugMap,
				Count
			};
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

		namespace ShaderTable {
			enum Type {
				E_RayGenShader = 0,
				E_SortedRayGenShader,
				E_MissShader,
				E_HitGroupShader,
				Count
			};
		}

		class RTAOClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL RaytracingSupported{};
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
				UINT ClientWidth{};
				UINT ClientHeight{};
			};

		public:
			RTAOClass();
			virtual ~RTAOClass() = default;

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
			virtual BOOL BuildShaderTables(UINT numRitems) override;

		public:
			BOOL DrawAO(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_GPU_VIRTUAL_ADDRESS accelStruct,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				Foundation::Resource::GpuResource* const pNormalDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
				Foundation::Resource::GpuResource* const pRayDirectionOriginDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_rayDirectionOriginDepthMap,
				Foundation::Resource::GpuResource* const pRayInexOffsetMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_rayIndexOffsetMap,
				BOOL bRaySortingEnabled,
				BOOL bCheckboardRayGeneration);

			UINT MoveToNextTemporalCacheFrame();
			UINT MoveToNextTemporalAOFrame();
			
		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature{};
			Microsoft::WRL::ComPtr<ID3D12StateObject> mStateObject{};
			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> mStateObjectProp{};

			std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, ShaderTable::Count> mShaderTables{};

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

			UINT mHitGroupShaderTableStrideInBytes{};

			UINT mCurrentTemporalCacheFrameIndex{};
			UINT mCurrentTemporalAOFrameIndex{};
		};

		using InitDataPtr = std::unique_ptr<RTAOClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "RTAO.inl"