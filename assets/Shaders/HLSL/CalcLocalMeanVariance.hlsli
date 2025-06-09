#ifndef __CALCLOCALMEANVARIANCE_HLSLI__
#define __CALCLOCALMEANVARIANCE_HLSLI__

namespace CalcLocalMeanVariance {
    // Adjust an index to a pixel that had a valid value generated for it.
    // Inactive pixel indices get increased by 1 in the y direction.
    int2 GetActivePixelIndex(int2 pixel) {
        const bool IsEvenPixel = ((pixel.x + pixel.y) & 1) == 0;
        return cbLocalMeanVar.CheckerboardSamplingEnabled && cbLocalMeanVar.EvenPixelActivated != IsEvenPixel ? pixel + int2(0, 1) : pixel;
    }
}

#endif // __CALCLOCALMEANVARIANCE_HLSLI__