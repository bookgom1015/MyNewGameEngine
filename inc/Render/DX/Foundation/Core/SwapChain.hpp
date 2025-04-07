#pragma once

#include <array>

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Foundation::Core {
	class SwapChain : public ShadingObject {
	public:
		static const UINT SwapChainBufferCount = 2;

	public:
		struct InitData {
			Factory* Factory;
			Device* Device;
			CommandObject* CommandObject;
			HWND		   MainWnd;
			UINT		   Width;
			UINT		   Height;
			BOOL		   AllowTearing;
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
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

		virtual BOOL BuildDescriptors(DescriptorHeap* const pDescHeap) override;
		virtual BOOL OnResize(UINT width, UINT height) override;

	private:
		BOOL CreateSwapChain();
		BOOL BuildSwapChainBuffers();
		BOOL BuildDescriptors();

	private:
		InitData mInitData;

		Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
		std::array<std::unique_ptr<Resource::GpuResource>, SwapChainBufferCount> mSwapChainBuffers;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferCpuSrvs;
		std::array<D3D12_GPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferGpuSrvs;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, SwapChainBufferCount> mhBackBufferCpuRtvs;

		UINT mCurrBackBuffer = 0;
	};
}