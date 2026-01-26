#ifndef __TAA_INL__
#define __TAA_INL__

namespace Render::DX11::Shading::TAA {
	constexpr UINT TAAClass::HaltonSequenceSize() const {
		return 16;
	}

	constexpr DirectX::XMFLOAT2 TAAClass::HaltonSequence(UINT index) const {
		return mFittedToBakcBufferHaltonSequence[index];
	}
}

#endif // __TAA_INL__