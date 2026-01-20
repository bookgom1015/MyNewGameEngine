#ifndef __UPLOADBUFFERWRAPPER_INL__
#define __UPLOADBUFFERWRAPPER_INL__

namespace Render::DX::Foundation::Resource {
	template <typename T>
	bool UploadBufferWrapper<T>::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Core::Device* const pDevice,
		UINT elementCount,
		UINT instanceCount,
		BOOL isConstantBuffer,
		LPCWSTR name) {
		return mUploadBuffer.Initialize(
			pLogFile,
			pDevice,
			elementCount,
			instanceCount,
			isConstantBuffer,
			name);
	}

	template <typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadBufferWrapper<T>::CBAddress() const {
		return mUploadBuffer.Resource()->GetGPUVirtualAddress();
	}

	template <typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadBufferWrapper<T>::CBAddress(UINT index) const {
		return mUploadBuffer.Resource()->GetGPUVirtualAddress() +
			static_cast<UINT64>(index) * static_cast<UINT64>(CBByteSize());
	}

	template <typename T>
	constexpr UINT UploadBufferWrapper<T>::CBByteSize() const noexcept {
		return (sizeof(T) + 255) & ~255;
	}

	template <typename T>
	void UploadBufferWrapper<T>::CopyCB(const T& data, INT elementIndex) {
		mUploadBuffer.CopyData(elementIndex, data);
	}
}

#endif // __UPLOADBUFFERWRAPPER_INL__