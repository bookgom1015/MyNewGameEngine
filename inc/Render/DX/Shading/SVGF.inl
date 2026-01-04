#ifndef __SVGF_INL__
#define __SVGF_INL__

UINT Render::DX::Shading::SVGF::NumMantissaBitsInFloatFormat(UINT floatFormatBitLength) {
	switch (floatFormatBitLength) {
	case 32: return 23;
	case 16: return 10;
	case 11: return 6;
	case 10: return 5;
	}
	return 0;
}

#endif // __SVGF_INL__