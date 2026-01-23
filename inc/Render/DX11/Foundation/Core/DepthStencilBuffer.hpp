#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"

namespace Render::DX11::Foundation::Core {
	class Device;

	class DepthStencilBuffer : public ShadingObject {
	public:
		struct InitData {
			UINT Width{};
			UINT Height{};
			Device* Device{};
		};

		static std::unique_ptr<InitData> MakeInitData();

	public:
		DepthStencilBuffer();
		virtual ~DepthStencilBuffer();

	public:
		__forceinline ID3D11DepthStencilView* DepthStencilView() noexcept;
		__forceinline ID3D11ShaderResourceView* DepthMapSrv() noexcept;

	public:
		virtual BOOL Initialize(
			Common::Debug::LogFile* const pLogFile, void* const pData) override;
		virtual void CleanUp() override;

		virtual BOOL OnResize(UINT width, UINT height) override;

	private:
		BOOL CreateDepthStencilBuffer();
		BOOL CreateDepthStencilBufferView();

	private:
		BOOL mbCleanedUp{};
		InitData mInitData;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilBuffer{};
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mhDepthStencilBufferView{};
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mhDepthMapSrv{};
	};
}

#include "DepthStencilBuffer.inl"