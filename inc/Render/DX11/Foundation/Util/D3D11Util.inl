#ifndef __D3D11UTIL_INL__
#define __D3D11UTIL_INL__

namespace Render::DX11::Foundation::Util {
	UINT D3D11Util::CeilDivide(UINT value, UINT divisor) { return (value + divisor - 1) / divisor; }
}

#endif // __D3D11UTIL_INL__