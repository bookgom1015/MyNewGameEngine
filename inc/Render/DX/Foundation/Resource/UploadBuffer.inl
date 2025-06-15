#ifndef __UPLOADBUFFER_INL__
#define __UPLOADBUFFER_INL__

template <typename T>
Render::DX::Foundation::Resource::UploadBuffer<T>::~UploadBuffer() {
	if (mUploadBuffer != nullptr) mUploadBuffer->Unmap(0, nullptr);
	mMappedData = nullptr;
}

template <typename T>
BOOL Render::DX::Foundation::Resource::UploadBuffer<T>::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Core::Device* const pDevice,
		UINT elementCount,
		UINT instanceCount,
		BOOL isConstantBuffer,
		LPCWSTR name) {
	mIsConstantBuffer = isConstantBuffer;
	mElementByteSize = sizeof(T);

	// Constant buffer elements need to be multiples of 256 bytes.
	// This is because the hardware can only view constant data 
	// at m*256 byte offsets and of n*256 byte lengths. 
	// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
	// UINT64 OffsetInBytes; // multiple of 256
	// UINT   SizeInBytes;   // multiple of 256
	// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
	if (isConstantBuffer)
		mElementByteSize = Util::D3D12Util::CalcConstantBufferByteSize(sizeof(T));

	CheckReturn(pLogFile, Util::D3D12Util::CreateUploadBuffer(pDevice, mElementByteSize * elementCount * instanceCount, IID_PPV_ARGS(&mUploadBuffer)));

	if (name != nullptr) CheckHRESULT(pLogFile, mUploadBuffer->SetName(name));

	if (FAILED(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData))))
		ReturnFalse(pLogFile, L"Failed mapping upload buffer");

	// We do not need to unmap until we are done with the resource.  However, we must not write to
	// the resource while it is in use by the GPU (so we must use synchronization techniques).

	return TRUE;
}

template <typename T>
ID3D12Resource* Render::DX::Foundation::Resource::UploadBuffer<T>::Resource() const {
	return mUploadBuffer.Get();
}

template <typename T>
void Render::DX::Foundation::Resource::UploadBuffer<T>::CopyData(INT elementIndex, const T& data) {
	std::memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
}

#endif // __UPLOADBUFFER_INL__