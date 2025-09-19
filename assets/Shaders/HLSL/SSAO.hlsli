#ifndef __SSAO_HLSLI__
#define __SSAO_HLSLI__

namespace SSAO {
    // Determines how much the sample point q occludes the point p as a function of distZ.
    float OcclusionFunction(in float dist, in float epsilon, in float fadeStart, in float fadeEnd) {
	//
	// If depth(q) is "behind" depth(p), then q cannot occlude p.  Moreover, if 
	// depth(q) and depth(p) are sufficiently close, then we also assume q cannot
	// occlude p because q needs to be in front of p by Epsilon to occlude p.
	//
	// We use the following function to determine the occlusion.  
	// 
	//
	//       1.0     -------------.
	//               |           |  .
	//               |           |    .
	//               |           |      . 
	//               |           |        .
	//               |           |          .
	//               |           |            .
	//  ------|------|-----------|-------------|---------|--> zv
	//        0     Eps          z0            z1        
	//
        float occlusion = 0.f;
        if (dist > epsilon) {
            const float FadeLength = fadeEnd - fadeStart;

			// Linearly decrease occlusion from 1 to 0 as distZ goes from gOcclusionFadeStart to gOcclusionFadeEnd.	
            occlusion = saturate((fadeEnd - dist) / FadeLength);
        }

        return occlusion;
    }
}

#endif // __SSAO_HLSLI__