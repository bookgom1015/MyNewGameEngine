#pragma once

#include <array>

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Foundation::Core {
	class SwapChain : public ShadingObject {
	public:
		static const UINT SwapChainBufferCount = 2;

	public:
		struct InitData {
			Common::Debug::LogFile* LogFile;
			IDXGIFactory4* DxgiFactory;
			ID3D12Device5* Device;
			ID3D12CommandQueue* CommandQueue;
			HWND MainWnd;
			UINT Width;
			UINT Height;
			BOOL AllowTearing;
		};

	public:
		SwapChain();
		virtual ~SwapChain();

	public:
		virtual UINT CbvSrvUavDescCount() const override;
		virtual UINT RtvDescCount() const override;
		virtual UINT DsvDescCount() const override;

	public:
		virtual BOOL Initialize(void* const pData) override;

		virtual BOOL BuildDescriptors(DescriptorHeap* const pDescHeap) override;
		virtual BOOL OnResize(UINT width, UINT height) override;

	private:
		BOOL CreateSwapChain();
		BOOL BuildSwapChainBuffers();
		BOOL BuildDescriptors();

	private:
		InitData mInitData;

		Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
		std::array<std::unique_ptr<Util::GpuResource>, SwapChainBufferCount> mSwapChainBuffers;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferCpuSrvs;
		std::array<D3D12_GPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferGpuSrvs;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferCpuRtvs;

		UINT mCurrBackBuffer = 0;
	};
}