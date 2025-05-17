#ifndef __SHADERTABLE_INL__
#define __SHADERTABLE_INL__

Microsoft::WRL::ComPtr<ID3D12Resource> Render::DX::Shading::Util::GpuUploadBuffer::GetResource() const {
	return mResource;
}

constexpr std::uint8_t* Render::DX::Shading::Util::ShaderTable::GetMappedShaderRecords() const {
	return mMappedShaderRecords;
}

constexpr UINT Render::DX::Shading::Util::ShaderTable::GetShaderRecordSize() const {
	return mShaderRecordSize;
}

#endif // __SHADERTABLE_INL__