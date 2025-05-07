#ifndef __TAA_INL__
#define __TAA_INL__

constexpr UINT Render::DX::Shading::TAA::TAAClass::HaltonSequenceSize() const {
	return 16;
}

constexpr DirectX::XMFLOAT2 Render::DX::Shading::TAA::TAAClass::HaltonSequence(UINT index) const {
	return mFittedToBakcBufferHaltonSequence[index];
}

#endif // __TAA_INL__