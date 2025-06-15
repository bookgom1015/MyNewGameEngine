#pragma once

#include "Render/DX/Foundation/Resource/UploadBuffer.hpp"

#include <vector>

namespace Render::DX::Foundation::Resource {
	template <typename T>
	class StructuredBuffer : public UploadBuffer<T> {
	public:
		StructuredBuffer() = default;
		virtual ~StructuredBuffer() = default;

	public:
		// Performance tip: Align structures on sizeof(float4) boundary.
		// Ref: https://developer.nvidia.com/content/understanding-structured-buffer-performance
		static_assert(sizeof(T) % 16 == 0, "Align structure buffers on 16 byte boundary for performance reasons.");

	public:
		T& operator[](UINT elementIndex);
		const T& operator[](UINT elementIndex) const;

	public:
		virtual BOOL Initialize(
			Common::Debug::LogFile* const pLogFile,
			Core::Device* const pDevice,
			UINT elementCount,
			UINT instanceCount,
			BOOL isConstantBuffer,
			LPCWSTR name = nullptr) override;

		void CopyStagingToGpu(UINT instanceIndex);

		D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT elementIndex, UINT instanceIndex);
		
	private:
		std::vector<T> mStaging;
		UINT mInstanceCount = 0;
	};
}

#include "StructuredBuffer.inl"