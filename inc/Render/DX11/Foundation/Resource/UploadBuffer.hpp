#pragma once

#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"

#ifndef ConstantBufferSize
#define ConstantBufferSize(__type) ((sizeof(__type) + 255) & ~255)
#endif

namespace Render::DX11::Foundation::Resource {
	template <typename T>
	class UploadBuffer {
	public:
		UploadBuffer() = default;
		virtual ~UploadBuffer();

	public:
		BOOL Initialize(
			Common::Debug::LogFile* const pLogFile, 
			Foundation::Core::Device* const pDevice, 
			UINT numElements);
		void CleanUp();

		void CopyData(const T& data, UINT index = 0);
		__forceinline ID3D11Buffer** CBAddress();

		BOOL BeginFrame();
		void EndFrame();

		BOOL SetData(const T& data);

		__forceinline constexpr UINT FirstConstant(UINT index = 0) const noexcept;
		__forceinline constexpr UINT NumConstants() const noexcept;

	protected:
		BOOL mbCleanedUp{};
		Common::Debug::LogFile* mpLogFile{};
		Foundation::Core::Device* mpDevice{};

		Microsoft::WRL::ComPtr<ID3D11Buffer> mUploadBuffer{};
		BYTE* mpMappedData{};

		UINT mElementByteSize{};
	};
}

#include "UploadBuffer.inl"