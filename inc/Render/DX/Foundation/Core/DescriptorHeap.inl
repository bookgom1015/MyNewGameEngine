#ifndef __DESCRIPTORHEAP_INL__
#define __DESCRIPTORHEAP_INL__

D3D12_CPU_DESCRIPTOR_HANDLE Render::DX::Foundation::Core::DescriptorHeap::CbvSrvUavCpuOffset(UINT offset) {
	return mhCpuCbvSrvUav.Offset(offset, mCbvSrvUavDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE Render::DX::Foundation::Core::DescriptorHeap::CbvSrvUavGpuOffset(UINT offset) {
	return mhGpuCbvSrvUav.Offset(offset, mCbvSrvUavDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Render::DX::Foundation::Core::DescriptorHeap::RtvCpuOffset(UINT offset) {
	return mhCpuRtv.Offset(offset, mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Render::DX::Foundation::Core::DescriptorHeap::DsvCpuOffset(UINT offset) {
	return mhCpuDsv.Offset(offset, mDsvDescriptorSize);
}

#endif // __DESCRIPTORHEAP_INL__