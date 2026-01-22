#ifndef __STRUCTUREDBUFFER_INL__
#define __STRUCTUREDBUFFER_INL__

namespace Render::DX::Foundation::Resource {
	template <typename T>
	T& StructuredBuffer<T>::operator[](UINT elementIndex) { return mStaging[elementIndex]; }

	template <typename T>
	const T& StructuredBuffer<T>::operator[](UINT elementIndex) const { return mStaging[elementIndex]; }

	template <typename T>
	BOOL StructuredBuffer<T>::Initialize(
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
	void StructuredBuffer<T>::CleanUp() {
		mStaging.clear();

		UploadBuffer<T>::CleanUp();
	}

	template <typename T>
	void StructuredBuffer<T>::CopyStagingToGpu(UINT instanceIndex) {
		const auto ElementSize = sizeof(T);
		const auto ElementCount = mStaging.size();

		std::memcpy(mMappedData + instanceIndex * ElementCount, &mStaging[0], ElementCount + ElementSize);
	}

	template <typename T>
	D3D12_GPU_VIRTUAL_ADDRESS StructuredBuffer<T>::GpuVirtualAddress(UINT elementIndex, UINT instanceIndex) {
		const auto ElementSize = sizeof(T);
		const auto ElementCount = mStaging.size();
		const auto InstanceSize = ElementSize * ElementCount;

		return mUploadBuffer->GetGPUVirtualAddress() + instanceIndex * InstanceSize + elementIndex * ElementSize;
	}
}

#endif // __STRUCTUREDBUFFER_INL__