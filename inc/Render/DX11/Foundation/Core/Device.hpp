#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace ImGuiManager::DX11 {
	class Dx11ImGuiManager;
}

namespace Render::DX11::Foundation {
	namespace Util {
		class D3D11Util;
	}

	namespace Core {
		class Device {
			friend class Util::D3D11Util;
			friend class ImGuiManager::DX11::Dx11ImGuiManager;

		public:
			Device();
			virtual ~Device();

		public:
			static void DumpRefCount(
				Common::Debug::LogFile* pLogFile, IUnknown* unk, const wchar_t* name);

		public:
			__forceinline ID3D11DeviceContext1* Context() noexcept;

		public:
			BOOL CreateSwapChainForHwnd(
				HWND hWnd,
				const DXGI_SWAP_CHAIN_DESC1* pDesc,
				const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
				IDXGIOutput* pRestrictToOutput,
				IDXGISwapChain1** ppSwapChain);

			// Descriptor view
			BOOL CreateRenderTargetView(
				ID3D11Resource* pResource,
				const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
				ID3D11RenderTargetView** ppRTView);
			BOOL CreateDepthStencilView(
				ID3D11Resource* pResource,
				const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
				ID3D11DepthStencilView** ppDepthStencilView);
			BOOL CreateShaderResourceView(
				ID3D11Resource* pResource,
				const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
				ID3D11ShaderResourceView** ppSRView);
			BOOL CreateUnorderedAccessView(
				ID3D11Resource* pResource,
				const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
				ID3D11UnorderedAccessView** ppUAView);

			// Resource
			BOOL CreateTexture2D(
				const D3D11_TEXTURE2D_DESC* pDesc,
				const D3D11_SUBRESOURCE_DATA* pInitialData,
				ID3D11Texture2D** ppTexture2D);
			BOOL CreateBuffer(
				const D3D11_BUFFER_DESC* pDesc,
				const D3D11_SUBRESOURCE_DATA* pInitialData,
				ID3D11Buffer** ppBuffer);

			BOOL CreateInputLayout(
				const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
				UINT NumElements,
				const void* pShaderBytecodeWithInputSignature,
				SIZE_T BytecodeLength,
				ID3D11InputLayout** ppInputLayout);

			// Pipeline State
			BOOL CreateRasterizerState(
				const D3D11_RASTERIZER_DESC* pRasterizerDesc,
				ID3D11RasterizerState** ppRasterizerState);
			BOOL CreateDepthStencilState(
				const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
				ID3D11DepthStencilState** ppDepthStencilState);
			BOOL CreateBlendState(
				const D3D11_BLEND_DESC* pBlendStateDesc,
				ID3D11BlendState** ppBlendState);
			BOOL CreateSamplerState(
				const D3D11_SAMPLER_DESC* pSamplerDesc,
				ID3D11SamplerState** ppSamplerState);

			// Shader
			BOOL CreateVertexShader(
				const void* pShaderBytecode,
				SIZE_T BytecodeLength,
				ID3D11ClassLinkage* pClassLinkage,
				ID3D11VertexShader** ppVertexShader);
			BOOL CreateGeometryShader(
				const void* pShaderBytecode,
				SIZE_T BytecodeLength,
				ID3D11ClassLinkage* pClassLinkage,
				ID3D11GeometryShader** ppGeometryShader);
			BOOL CreatePixelShader(
				const void* pShaderBytecode,
				SIZE_T BytecodeLength,
				ID3D11ClassLinkage* pClassLinkage,
				ID3D11PixelShader** ppPixelShader);
			BOOL CreateComputeShader(
				const void* pShaderBytecode,
				SIZE_T BytecodeLength,
				ID3D11ClassLinkage* pClassLinkage,
				ID3D11ComputeShader** ppComputeShader);

		public:
			BOOL Initialize(Common::Debug::LogFile* const pLogFile);
			void CleanUp();

			BOOL FlushDebugMessages();
			void Flush();

		private:
			BOOL CreateDevice();
			BOOL CreateFactory();
			BOOL SetBreakOnSeverity();

			BOOL FlushDXGIMessages();
			BOOL FlushD3D11DebugMessages();

			void ReportLiveObjects();

		private:
			BOOL mbCleanedUp{};
			Common::Debug::LogFile* mpLogFile{};

		private:
			Microsoft::WRL::ComPtr<ID3D11Device> mDevice{};
			Microsoft::WRL::ComPtr<IDXGIFactory2> mFactory{};
			Microsoft::WRL::ComPtr<ID3D11DeviceContext1> mContext{};
		};
	}
}

#include "Device.inl"