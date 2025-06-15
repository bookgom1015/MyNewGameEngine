#ifndef __STRUCTUREDBUFFER_INL__
#define __STRUCTUREDBUFFER_INL__

template <typename T>
T& Render::DX::Foundation::Resource::StructuredBuffer<T>::operator[](UINT elementIndex) { return mStaging[elementIndex]; }

template <typename T>
const T& Render::DX::Foundation::Resource::StructuredBuffer<T>::operator[](UINT elementIndex) const { return m_staging[elementIndex]; }

template <typename T>
BOOL Render::DX::Foundation::Resource::StructuredBuffer<T>::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Core::Device* const pDevice,
		UINT elementCount,
		UINT instanceCount,
		BOOL isConstantBuffer,
		LPCWSTR name) {
	CheckReturn(pLogFile, UploadBuffer<T>::Initialize(pLogFile, pDevice, elementCount, instanceCount, isConstantBuffer, name));

	mStaging.resize(elementCount);

	return TRUE;
}

template <typename T>
void Render::DX::Foundation::Resource::StructuredBuffer<T>::CopyStagingToGpu(UINT instanceIndex) {
	const auto ElementSize = sizeof(T);
	const auto ElementCount = mStaging.size();

	std::memcpy(mMappedData + instanceIndex * ElementCount, &mStaging[0], ElementCount + ElementSize);
}

template <typename T>
D3D12_GPU_VIRTUAL_ADDRESS Render::DX::Foundation::Resource::StructuredBuffer<T>::GpuVirtualAddress(UINT elementIndex, UINT instanceIndex) {
	const auto ElementSize = sizeof(T);
	const auto ElementCount = mStaging.size();
	const auto InstanceSize = ElementSize * ElementCount;

	return mUploadBuffer->GetGPUVirtualAddress() + instanceIndex * InstanceSize + elementIndex * ElementSize;
}

#endif // __STRUCTUREDBUFFER_INL__