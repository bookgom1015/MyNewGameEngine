#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

#include <dxgi1_6.h>
#include <array>

namespace Render::DX::Foundation::Core {
	class SwapChain : public ShadingObject {
	public:
		static const UINT SwapChainBufferCount = 2;

	public:
		struct InitData {
			Factory*		Factory;
			Device*			Device;
			CommandObject*	CommandObject;
			HWND			MainWnd;
			UINT			ClientWidth;
			UINT			ClientHeight;
			BOOL			AllowTearing;
		};

		using InitDataPtr = std::unique_ptr<InitData>;

	public:
		SwapChain();
		virtual ~SwapChain();

	public:
		virtual UINT CbvSrvUavDescCount() const override;
		virtual UINT RtvDescCount() const override;
		virtual UINT DsvDescCount() const override;

	public:
		static InitDataPtr MakeInitData();

	public:
		__forceinline Resource::GpuResource* BackBuffer() const;
		__forceinline D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRtv() const;

		__forceinline Resource::GpuResource* BackBufferCopy() const;
		__forceinline D3D12_GPU_DESCRIPTOR_HANDLE BackBufferCopySrv() const;

		__forceinline constexpr D3D12_VIEWPORT ScreenViewport() const;
		__forceinline constexpr D3D12_RECT ScissorRect() const;

	public:
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

		virtual BOOL BuildDescriptors(DescriptorHeap* const pDescHeap) override;
		virtual BOOL OnResize(UINT width, UINT height) override;

	public:
		BOOL ReadyToPresent(Resource::FrameResource* const pFrameResource);
		BOOL Present(BOOL bAllowTearing);
		void NextBackBuffer();

	private:
		BOOL CreateSwapChain();
		BOOL BuildSwapChainBuffers();
		BOOL BuildResources();
		BOOL BuildDescriptors();

	private:
		InitData mInitData{};

		Microsoft::WRL::ComPtr<IDXGISwapChain1> mSwapChain{};
		std::array<std::unique_ptr<Resource::GpuResource>, SwapChainBufferCount> mSwapChainBuffers{};
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferCpuSrvs{};
		std::array<D3D12_GPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferGpuSrvs{};
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferCpuRtvs{};

		std::unique_ptr<Resource::GpuResource> mBackBufferCopy{};
		D3D12_CPU_DESCRIPTOR_HANDLE mhBackBufferCopyCpuSrv{};
		D3D12_GPU_DESCRIPTOR_HANDLE mhBackBufferCopyGpuSrv{};

		UINT mCurrBackBuffer{};

		D3D12_VIEWPORT mScreenViewport{};
		D3D12_RECT mScissorRect{};
	};
}

#include "Render/DX/Foundation/Core/SwapChain.inl"