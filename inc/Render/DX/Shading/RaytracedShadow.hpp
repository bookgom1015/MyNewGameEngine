#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace RaytracedShadow {
		namespace Shader {
			enum Type {
				Lib_RaytracedShadow = 0,
				Count
			};
		}

		namespace RootSignature {
			enum {
				CB_Light = 0,
				SI_AccelerationStructure,
				SI_PositionMap,
				SI_NormalMap,
				SI_DepthMap,
				UO_ShadowMap,
				Count
			};
		}

		namespace ShaderTable {
			enum Type {
				E_RayGenShader = 0,
				E_MissShader,
				E_HitGroupShader,
				Count
			};
		}

		class RaytracedShadowClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL RaytracingSupported = FALSE;
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
				UINT ClientWidth = 0;
				UINT ClientHeight = 0;
			};

		public:
			RaytracedShadowClass();
			virtual ~RaytracedShadowClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* ShadowMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE ShadowMapSrv() const;

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
			BOOL CalcShadow(
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

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature{};
			Microsoft::WRL::ComPtr<ID3D12StateObject> mStateObject{};
			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> mStateObjectProp{};

			std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, ShaderTable::Count> mShaderTables{};

			std::unique_ptr<Foundation::Resource::GpuResource> mShadowMap{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhShadowMapCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhShadowMapGpuSrv{};
			D3D12_CPU_DESCRIPTOR_HANDLE mhShadowMapCpuUav{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhShadowMapGpuUav{};

			UINT mHitGroupShaderTableStrideInBytes{};
		};

		using InitDataPtr = std::unique_ptr<RaytracedShadowClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

namespace Render::DX::Shading::RaytracedShadow {
	Foundation::Resource::GpuResource* RaytracedShadowClass::ShadowMap() const {
		return mShadowMap.get();
	}

	constexpr D3D12_GPU_DESCRIPTOR_HANDLE RaytracedShadowClass::ShadowMapSrv() const {
		return mhShadowMapGpuSrv;
	}
}