#ifndef __UPLOADBUFFER_INL__
#define __UPLOADBUFFER_INL__

namespace Render::DX11::Foundation::Resource {
	template <typename T>
	UploadBuffer<T>::~UploadBuffer() { CleanUp(); }

	template <typename T>
	BOOL UploadBuffer<T>::Initialize(
			Common::Debug::LogFile* const pLogFile,
			Foundation::Core::Device* const pDevice, 
			UINT numElements) {
		mpLogFile = pLogFile;
		mpDevice = pDevice;

		mElementByteSize = ConstantBufferSize(T);

		D3D11_BUFFER_DESC desc{};
		desc.ByteWidth = mElementByteSize * numElements;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		CheckReturn(mpLogFile, mpDevice->CreateBuffer(&desc, nullptr, &mUploadBuffer));

		return true;
	}

	template <typename T>
	void UploadBuffer<T>::CleanUp() {
		if (mbCleanedUp) return;

		mUploadBuffer.Reset();

		mbCleanedUp = TRUE;
	}

	template <typename T>
	void UploadBuffer<T>::CopyData(const T& data, UINT index) {
		std::memcpy(&mpMappedData[index * mElementByteSize], &data, sizeof(T));
	}

	template <typename T>
	ID3D11Buffer** UploadBuffer<T>::CBAddress() { return mUploadBuffer.GetAddressOf(); }

	template <typename T>
	BOOL UploadBuffer<T>::BeginFrame() {
		D3D11_MAPPED_SUBRESOURCE mapped{};
		CheckHRESULT(mpLogFile, mpDevice->Context()->Map(
			mUploadBuffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));

		mpMappedData = reinterpret_cast<BYTE*>(mapped.pData);

		return true;
	}

	template <typename T>
	void UploadBuffer<T>::EndFrame() {
		mpDevice->Context()->Unmap(mUploadBuffer.Get(), 0);
		mpMappedData = nullptr;
	}

	template <typename T>
	BOOL UploadBuffer<T>::SetData(const T& data) {
		D3D11_MAPPED_SUBRESOURCE mapped{};
		CheckHRESULT(mpLogFile, mpDevice->Context()->Map(
			mUploadBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));

		std::memcpy(mapped.pData, &data, sizeof(T));

		mpDevice->Context()->Unmap(mUploadBuffer.Get(), 0);

		return TRUE;
	}

	template <typename T>
	constexpr UINT UploadBuffer<T>::FirstConstant(UINT index) const noexcept {
		return (mElementByteSize * index) / 16;
	}

	template <typename T>
	constexpr UINT UploadBuffer<T>::NumConstants() const noexcept {
		return mElementByteSize / 16;
	}
}

#endif // __UPLOADBUFFER_INL__