#ifndef __D3D12UTIL_INL__
#define __D3D12UTIL_INL__

UINT Render::DX::Foundation::Util::D3D12Util::CeilDivide(UINT value, UINT divisor) {
	return (value + divisor - 1) / divisor;
}

#endif // __D3D12UTIL_INL__