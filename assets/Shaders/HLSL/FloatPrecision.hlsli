#ifndef __FLOATPRECISION_HLSLI__
#define __FLOATPRECISION_HLSLI__

namespace FloatPrecision {
    uint SmallestPowerOf2GreaterThan(in uint x) {
	    // Set all the bits behind the most significant non-zero bit in x to 1.
	    // Essentially giving us the largest value that is smaller than the
	    // next power of 2 we're looking for.
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;

	    // Return the next power of two value.
        return x + 1;
    }

    // Returns float precision for a given float value.
    // Values within (value -precision, value + precision) map to the same value. 
    // Precision = exponentRange/MaxMantissaValue = (2^e+1 - 2^e) / (2^NumMantissaBits)
    // Ref: https://blog.demofox.org/2017/11/21/floating-point-precision/
    float FloatPrecision(in float x, in uint NumMantissaBits) {
	    // Find the exponent range the value is in.
        const uint nextPowerOfTwo = SmallestPowerOf2GreaterThan(x);
        const float exponentRange = nextPowerOfTwo - (nextPowerOfTwo >> 1);

        const float MaxMantissaValue = 1u << NumMantissaBits;

        return exponentRange / MaxMantissaValue;
    }

    float FloatPrecisionR10(in float x) {
        return FloatPrecision(x, 5);
    }

    float FloatPrecisionR16(in float x) {
        return FloatPrecision(x, 10);
    }

    float FloatPrecisionR32(in float x) {
        return FloatPrecision(x, 23);
    }
}

#endif // __FLOATPRECISION_HLSLI__