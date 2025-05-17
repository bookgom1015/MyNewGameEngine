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
				SI_NormalMap,
				SI_DepthMap,
				UO_AOCoefficientMap,
				UO_RayHitDistanceMap,
				Count
			};
		}

		namespace Resource {
			namespace AO {
				enum {
					E_AOCoefficient = 0,
					E_RayHitDistance,
					Count
				};
			}

			namespace TemporalCache {
				enum {
					E_TSPP = 0,
					E_RayHitDistance,
					E_AOCoefficientSquaredMean,
					E_AOCoefficient,
					Count
				};
			}
		}

		namespace Descriptor {
			namespace AO {
				enum {
					ES_AOCoefficient = 0,
					EU_AOCoefficient,
					ES_RayHitDistance,
					EU_RayHitDistance,
					Count
				};
			}

			namespace TemporalCache {
				enum {
					ES_TSPP = 0,
					EU_TSPP,
					ES_RayHitDistance,
					EU_RayHitDistance,
					ES_AOCoefficientSquaredMean,
					EU_AOCoefficientSquaredMean,
					ES_AOCoefficient,
					EU_AOCoefficient,
					Count
				};
			}
		}

		namespace ShaderTable {
			enum Type {
				E_RayGenShader = 0,
				E_MissShader,
				E_HitGroupShader,
				Count
			};
		}

		class RTAOClass : public Foundation::ShadingObject {
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
			RTAOClass();
			virtual ~RTAOClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* AOCoefficientMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE AOCoefficientMapSrv() const;

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
			virtual BOOL BuildShaderTables(UINT numRitems) override;

		public:
			BOOL DrawAO(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_GPU_VIRTUAL_ADDRESS accelStruct,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
				Foundation::Resource::GpuResource* const pNormalMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
				Foundation::Resource::GpuResource* const pDepthMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			Microsoft::WRL::ComPtr<ID3D12StateObject> mStateObject;
			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> mStateObjectProp;

			std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, ShaderTable::Count> mShaderTables;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::AO::Count> mAOResources;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Descriptor::AO::Count> mhAOResourceCpus;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Descriptor::AO::Count> mhAOResourceGpus;

			std::array<std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::TemporalCache::Count>, 2> mTemporalCaches;
			std::array<std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Descriptor::TemporalCache::Count>, 2> mhTemporalCacheCpus;
			std::array<std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Descriptor::TemporalCache::Count>, 2> mhTemporalCacheGpus;

			UINT mHitGroupShaderTableStrideInBytes = 0;
		};

		using InitDataPtr = std::unique_ptr<RTAOClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

Render::DX::Foundation::Resource::GpuResource* Render::DX::Shading::RTAO::RTAOClass::AOCoefficientMap() const {
	return mAOResources[Resource::AO::E_AOCoefficient].get();
}

constexpr D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Shading::RTAO::RTAOClass::AOCoefficientMapSrv() const {
	return mhAOResourceGpus[Descriptor::AO::ES_AOCoefficient];
}