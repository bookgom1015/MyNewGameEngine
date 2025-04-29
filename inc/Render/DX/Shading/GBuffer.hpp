#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace GBuffer {
		namespace Shader {
			enum Type {
				VS_GBuffer = 0,
				MS_GBuffer,
				PS_GBuffer,
				Count
			};
		}

		namespace RootSignature {
			namespace Default {
				enum {
					CB_Pass = 0,
					CB_Object,
					CB_Material,
					RC_Consts,
					SI_VertexBuffer,
					SI_IndexBuffer,
					SI_Textures,
					Count
				};
			}
		}

		class GBufferClass : public Foundation::ShadingObject {
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
			GBufferClass();
			virtual ~GBufferClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* AlbedoMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE AlbedoMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* NormalMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE NormalMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* SpecularMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE SpecularMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* RoughnessMetalnessMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE RoughnessMetalnessMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* VelocityMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE VelocityMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* PositionMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE PositionMapSrv() const;

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
			BOOL DrawGBuffer(
				Foundation::Resource::FrameResource* const pFrameResource,
				D3D12_VIEWPORT viewport, 
				D3D12_RECT scissorRect,
				Foundation::Resource::GpuResource* const backBuffer, 
				D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
				Foundation::Resource::GpuResource* const depthBuffer, 
				D3D12_CPU_DESCRIPTOR_HANDLE do_depthBuffer,
				const std::vector<Render::DX::Foundation::RenderItem*>& ritems);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

			BOOL DrawRenderItems(
				Foundation::Resource::FrameResource* const pFrameResource,
				ID3D12GraphicsCommandList6* const pCmdList,
				const std::vector<Render::DX::Foundation::RenderItem*>& ritems);

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;

			std::unique_ptr<Foundation::Resource::GpuResource> mAlbedoMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhAlbedoMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhAlbedoMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhAlbedoMapCpuRtv;

			std::unique_ptr<Foundation::Resource::GpuResource> mNormalMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuRtv;

			std::unique_ptr<Foundation::Resource::GpuResource> mSpecularMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhSpecularMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhSpecularMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhSpecularMapCpuRtv;

			std::unique_ptr<Foundation::Resource::GpuResource> mRoughnessMetalnessMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhRoughnessMetalnessMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhRoughnessMetalnessMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhRoughnessMetalnessMapCpuRtv;

			std::unique_ptr<Foundation::Resource::GpuResource> mVelocityMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhVelocityMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhVelocityMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhVelocityMapCpuRtv;

			std::unique_ptr<Foundation::Resource::GpuResource> mPositionMap;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhPositionMapCpuSrv;
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhPositionMapGpuSrv;
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhPositionMapCpuRtv;
		};

		using InitDataPtr = std::unique_ptr<GBufferClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "GBuffer.inl"