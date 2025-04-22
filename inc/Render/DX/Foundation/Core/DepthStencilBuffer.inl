#ifndef __DEPTHSTENCILBUFFER_INL__
#define __DEPTHSTENCILBUFFER_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Foundation::Core::DepthStencilBuffer::GetDepthStencilBuffer() const {
	return mDepthStencilBuffer.get();
}

D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Foundation::Core::DepthStencilBuffer::DepthStencilBufferSrv() const {
	return mhDepthStencilBufferGpuSrv;
}

D3D12_CPU_DESCRIPTOR_HANDLE Render::DX::Foundation::Core::DepthStencilBuffer::DepthStencilBufferDsv() const {
	return mhDepthStencilBufferCpuDsv;
}

#endif // __DEPTHSTENCILBUFFER_INL__