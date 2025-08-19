//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef __RANDOM_HLSLI__
#define __RANDOM_HLSLI__

#ifndef MAX_HALTON_SEQUENCE
#define MAX_HALTON_SEQUENCE 16
#endif 

namespace Random {
    static const float3 HaltonSequence[MAX_HALTON_SEQUENCE] = {
	    float3(0.5f,     0.333333f, 0.2f),
	    float3(0.25f,    0.666667f, 0.4f),
	    float3(0.75f,    0.111111f, 0.6f),
	    float3(0.125f,   0.444444f, 0.8f),
	    float3(0.625f,   0.777778f, 0.04f),
	    float3(0.375f,   0.222222f, 0.24f),
	    float3(0.875f,   0.555556f, 0.44f) ,
	    float3(0.0625f,  0.888889f, 0.64f),
	    float3(0.5625f,  0.037037f, 0.84f),
	    float3(0.3125f,  0.37037f,  0.08f),
	    float3(0.8125f,  0.703704f, 0.28f),
	    float3(0.1875f,  0.148148f, 0.48f),
	    float3(0.6875f,  0.481482f, 0.68f),
	    float3(0.4375f,  0.814815f, 0.88f),
	    float3(0.9375f,  0.259259f, 0.12f),
	    float3(0.03125f, 0.592593f, 0.32f)
    };

    // Generates a seed for a random number generator from 2 inputs plus a backoff
    uint InitRand(uint val0, uint val1, uint backoff = 16) {
        uint v0 = val0;
        uint v1 = val1;
        uint s0 = 0;

	    [unroll]
        for (uint n = 0; n < backoff; ++n) {
            s0 += 0x9e3779b9;
            v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
            v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
        }

        return v0;
    }
    
    // Takes our seed, updates it, and returns a pseudorandom float in [0..1]
    float NextRand(inout uint s) {
        s = (1664525u * s + 1013904223u);
        return float(s & 0x00FFFFFF) / float(0x01000000);
    }

    // Utility function to get a vector perpendicular to an input vector 
    //    (from "Efficient Construction of Perpendicular Vectors Without Branching")
    float3 PerpendicularVector(float3 u) {
        float3 a = abs(u);
        uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
        uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
        uint zm = 1 ^ (xm | ym);
        return cross(u, float3(xm, ym, zm));
    }

    // Get a cosine-weighted random vector centered around a specified normal direction.
    float3 CosHemisphereSample(inout uint seed, float3 hitNorm) {
	    // Get 2 random numbers to select our sample with
        float2 randVal = float2(NextRand(seed), NextRand(seed));

	    // Cosine weighted hemisphere sample from RNG
        float3 bitangent = PerpendicularVector(hitNorm);
        float3 tangent = cross(bitangent, hitNorm);
        float r = sqrt(randVal.x);
        float phi = 2.0f * 3.14159265f * randVal.y;

	    // Get our cosine-weighted hemisphere lobe sample direction
        return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
    }
    
    // Create an initial random number for this thread
    uint SeedThread(in uint seed) {
        // Thomas Wang hash 
        // Ref: http://www.burtleburtle.net/bob/hash/integer.html
        seed = (seed ^ 61) ^ (seed >> 16);
        seed *= 9;
        seed = seed ^ (seed >> 4);
        seed *= 0x27d4eb2d;
        seed = seed ^ (seed >> 15);
        return seed;
    }

    // Generate a random 32-bit integer
    uint Random(inout uint state) {
        // Xorshift algorithm from George Marsaglia's paper.
        state ^= (state << 13);
        state ^= (state >> 17);
        state ^= (state << 5);
        return state;
    }

    // Generate a random float in the range [0.0f, 1.0f)
    float Random01(inout uint state) {
        return asfloat(0x3f800000 | Random(state) >> 9) - 1.0;
    }

    // Generate a random float in the range [0.0f, 1.0f]
    float Random01inclusive(inout uint state) {
        return Random(state) / float(0xffffffff);
    }


    // Generate a random integer in the range [lower, upper]
    uint Random(inout uint state, in uint lower, in uint upper) {
        return lower + uint(float(upper - lower + 1) * Random01(state));
    }
}

#endif // __RANDOM_HLSLI__