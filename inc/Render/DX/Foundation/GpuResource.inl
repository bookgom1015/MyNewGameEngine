#ifndef __GPURESOURCE_INL__
#define __GPURESOURCE_INL__

void Render::DX::Foundation::GpuResource::Reset() {
	mResource.Reset();
}

ID3D12Resource* const Render::DX::Foundation::GpuResource::Resource() const {
	return mResource.Get();
}

D3D12_RESOURCE_DESC Render::DX::Foundation::GpuResource::Desc() const {
	return mResource->GetDesc();
}

D3D12_RESOURCE_STATES Render::DX::Foundation::GpuResource::State() const {
	return mCurrState;
}

#endif // __GPURESOURCE_INL__