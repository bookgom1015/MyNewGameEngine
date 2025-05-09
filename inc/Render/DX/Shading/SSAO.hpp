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
					CB_SSAO = 0,
					RC_Consts,
					SI_NormalMap,
					SI_PositionMap,
					SI_RandomVectorMap,
					UO_AOMap,
					Count
				};
			}
		}

		class SSAOClass : public Foundation::ShadingObject {
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
			SSAOClass();
			virtual ~SSAOClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* AOMap(UINT index) const;
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

			BOOL DrawSSAO(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::Resource::GpuResource* const pNormalMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
				Foundation::Resource::GpuResource* const pPositionMap,
				D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap);

		private:
			BOOL BuildRandomVectorMapResource();
			BOOL BuildRandomVectorMapDescriptor();

			BOOL BuildAOMapResources();
			BOOL BuildAOMapDescriptors();

			void BuildOffsetVecotrs();

		private:
			InitData mInitData;

			D3D12_VIEWPORT mViewport;
			D3D12_RECT mScissorRect;

			UINT mTexWidth = 0;
			UINT mTexHeight = 0;

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			std::unique_ptr<Foundation::Resource::GpuResource> mRandomVectorMap;
			std::unique_ptr<Foundation::Resource::GpuResource> mRandomVectorMapUploadBuffer;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhRandomVectorMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhRandomVectorMapGpuSrv;

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, 2> mAOMaps;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> mhAOMapCpuSrvs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2> mhAOMapGpuSrvs;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> mhAOMapCpuUavs;
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2> mhAOMapGpuUavs;

			DirectX::XMFLOAT4 mOffsets[14];
		};

		using InitDataPtr = std::unique_ptr<SSAOClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "SSAO.inl"