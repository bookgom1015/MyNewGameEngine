#pragma once

#include "Render/DX/Foundation/ConstantBuffer.h"
#include "Render/DX/Foundation/Resource/UploadBuffer.hpp"

namespace Render::DX::Foundation::Resource {
	template <typename T>
	class UploadBufferWrapper {
	public:
		__forceinline bool Initialize(
			Common::Debug::LogFile* const pLogFile,
			Core::Device* const pDevice,
			UINT elementCount,
			UINT instanceCount,
			BOOL isConstantBuffer,
			LPCWSTR name = nullptr);

		__forceinline D3D12_GPU_VIRTUAL_ADDRESS CBAddress() const;
		__forceinline D3D12_GPU_VIRTUAL_ADDRESS CBAddress(UINT index) const;
		__forceinline constexpr UINT CBByteSize() const noexcept;

		__forceinline void CopyCB(const T& data, INT elementIndex = 0);

	private:
		UploadBuffer<T> mUploadBuffer{};
	};
}

#include "UploadBufferWrapper.inl"