#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Foundation::Core {
	class DepthStencilBuffer : public ShadingObject {
	public:
		struct InitData {
			Device* Device;
			UINT	ClientWidth;
			UINT	ClientHeight;
		};

		using InitDataPtr = std::unique_ptr<InitData>;

	public:
		DepthStencilBuffer();
		virtual ~DepthStencilBuffer();

	public:
		virtual UINT CbvSrvUavDescCount() const override;
		virtual UINT RtvDescCount() const override;
		virtual UINT DsvDescCount() const override;

	public:
		static InitDataPtr MakeInitData();

	public:
		__forceinline Resource::GpuResource* GetDepthStencilBuffer() const;
		__forceinline D3D12_GPU_DESCRIPTOR_HANDLE DepthStencilBufferSrv() const;
		__forceinline D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBufferDsv() const;

	public:
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

		virtual BOOL BuildDescriptors(DescriptorHeap* const pDescHeap) override;
		virtual BOOL OnResize(UINT width, UINT height) override;

	private:
		BOOL BuildDepthStencilBuffer();
		BOOL BuildDescriptors();

	private:
		InitData mInitData{};

		std::unique_ptr<Resource::GpuResource> mDepthStencilBuffer{};
		D3D12_CPU_DESCRIPTOR_HANDLE mhDepthStencilBufferCpuSrv{};
		D3D12_GPU_DESCRIPTOR_HANDLE mhDepthStencilBufferGpuSrv{};
		D3D12_CPU_DESCRIPTOR_HANDLE mhDepthStencilBufferCpuDsv{};
	};
}

#include "DepthStencilBuffer.inl"