#ifndef __BLURFILTER_INL__
#define __BLURFILTER_INL__

INT Render::DX::Shading::BlurFilter::CalcDiameter(FLOAT sigma) {
	const INT BlurRadius = static_cast<INT>(ceil(2.f * sigma));
	return 2 * BlurRadius + 1;
}

BOOL Render::DX::Shading::BlurFilter::CalcGaussWeights(FLOAT sigma, FLOAT weights[]) {
	FLOAT twoSigma2 = 2.f * sigma * sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	const INT BlurRadius = static_cast<INT>(ceil(2.f * sigma));

	const INT Size = 2 * BlurRadius + 1;
	FLOAT weightSum = 0.f;

	for (INT i = -BlurRadius; i <= BlurRadius; ++i) {
		FLOAT x = static_cast<FLOAT>(i);

		weights[i + BlurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + BlurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (INT i = 0; i < Size; ++i)
		weights[i] /= weightSum;

	return TRUE;
}

#endif // __BLURFILTER_INL__