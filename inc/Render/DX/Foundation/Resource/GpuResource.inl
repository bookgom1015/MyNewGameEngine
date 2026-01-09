#ifndef __GPURESOURCE_INL__
#define __GPURESOURCE_INL__

void Render::DX::Foundation::Resource::GpuResource::Reset() {
	mResource.Reset();
}

ID3D12Resource* Render::DX::Foundation::Resource::GpuResource::Resource() const noexcept {
	return mResource.Get();
}

D3D12_RESOURCE_DESC Render::DX::Foundation::Resource::GpuResource::Desc() const {
	return mResource->GetDesc();
}

D3D12_RESOURCE_STATES Render::DX::Foundation::Resource::GpuResource::State() const {
	return mCurrState;
}

#endif // __GPURESOURCE_INL__