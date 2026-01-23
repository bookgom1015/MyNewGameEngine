#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11::Foundation::Core {
	class Device;

	class SwapChain : public ShadingObject {
	public:
		static const UINT SwapChainBufferCount = 2;

	public:
		struct InitData {
			UINT Width{};
			UINT Height{};
			HWND WindowHandle{};
			Device* Device{};
		};

		static std::unique_ptr<InitData> MakeInitData();

	public:
		SwapChain();
		virtual ~SwapChain();

	public:
		__forceinline constexpr D3D11_VIEWPORT& ScreenViewport() noexcept;
		__forceinline constexpr const D3D11_VIEWPORT& ScreenViewport() const noexcept;

		__forceinline ID3D11Texture2D* SwapChainBuffer();
		__forceinline ID3D11RenderTargetView* SwapChainBufferRtv() noexcept;

		__forceinline ID3D11Texture2D* SwapChainBufferCopy();
		__forceinline ID3D11ShaderResourceView* SwapChainBufferCopySrv() noexcept;

	public:
		virtual BOOL Initialize(
			Common::Debug::LogFile* const pLogFile, void* const pData) override;
		virtual void CleanUp() override;

		virtual BOOL OnResize(UINT width, UINT height) override;

	public:
		BOOL Present();

	private:
		BOOL CreateSwapChain();
		BOOL CreateSwapChainBuffer();
		BOOL CreateSwapChainBufferView();

	private:
		BOOL mbCleanedUp{};
		InitData mInitData{};

		Microsoft::WRL::ComPtr<IDXGISwapChain1> mSwapChain{};

		Microsoft::WRL::ComPtr<ID3D11Texture2D> mSwapChainBuffer{};
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mhSwapChainBufferRtv{};

		Microsoft::WRL::ComPtr<ID3D11Texture2D> mSwapChainBufferCopy{};
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhSwapChainBufferCopySrv{};

		D3D11_VIEWPORT mScreenViewport{};
	};
}

#include "SwapChain.inl"