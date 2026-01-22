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

		namespace PipelineState {
			enum Type {
				GP_GBuffer = 0,
				MP_GBuffer,
				Count
			};
		}

		namespace Resource {
			enum Type {
				E_Albedo = 0,
				E_Normal,
				E_NormalDepth,
				E_ReprojNormalDepth,
				E_CachedNormalDepth,
				E_Specular,
				E_RoughnessMetalness,
				E_Velcity,
				E_Position,
				Count
			};
		}

		namespace Descriptor {
			namespace Srv {
				enum Type {
					E_Albedo = 0,
					E_Normal,
					E_NormalDepth,
					E_ReprojNormalDepth,
					E_CachedNormalDepth,
					E_Specular,
					E_RoughnessMetalness,
					E_Velocity,
					E_Position,
					Count
				};
			}

			namespace Rtv {
				enum Type {
					E_Albedo = 0,
					E_Normal,
					E_NormalDepth,
					E_ReprojNormalDepth,
					E_Specular,
					E_RoughnessMetalness,
					E_Velocity,
					E_Position,
					Count
				};
			}
		}

		class GBufferClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL MeshShaderSupported{};
				Foundation::Core::Device* Device{};
				Foundation::Core::CommandObject* CommandObject{};
				Foundation::Core::DescriptorHeap* DescriptorHeap{};
				Util::ShaderManager* ShaderManager{};
				UINT ClientWidth{};
				UINT ClientHeight{};
			};

		public:
			GBufferClass();
			virtual ~GBufferClass() = default;

		public:
			__forceinline Foundation::Resource::GpuResource* AlbedoMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE AlbedoMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* NormalMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE NormalMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* NormalDepthMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE NormalDepthMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* ReprojectedNormalDepthMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE ReprojectedNormalDepthMapSrv() const;

			__forceinline Foundation::Resource::GpuResource* CachedNormalDepthMap() const;
			__forceinline constexpr D3D12_GPU_DESCRIPTOR_HANDLE CachedNormalDepthMapSrv() const;

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
			virtual void CleanUp() override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures() override;
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
				const std::vector<Render::DX::Foundation::RenderItem*>& ritems,
				FLOAT ditheringMaxDist, FLOAT ditheringMinDist);

		private:
			BOOL BuildResources();
			BOOL BuildDescriptors();

			BOOL DrawRenderItems(
				Foundation::Resource::FrameResource* const pFrameResource,
				ID3D12GraphicsCommandList6* const pCmdList,
				const std::vector<Render::DX::Foundation::RenderItem*>& ritems,
				FLOAT ditheringMaxDist, FLOAT ditheringMinDist);
			BOOL CacheNormalDepth(ID3D12GraphicsCommandList6* const pCmdList);

		private:
			InitData mInitData{};

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes{};

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature{};
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates{};

			std::array<std::unique_ptr<Foundation::Resource::GpuResource>, Resource::Count> mResources{};
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Descriptor::Srv::Count> mhCpuSrvs{};
			std::array<D3D12_GPU_DESCRIPTOR_HANDLE, Descriptor::Srv::Count> mhGpuSrvs{};
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, Descriptor::Rtv::Count> mhCpuRtvs{};		
		};

		using InitDataPtr = std::unique_ptr<GBufferClass::InitData>;

		InitDataPtr MakeInitData();
	}
}

#include "GBuffer.inl"