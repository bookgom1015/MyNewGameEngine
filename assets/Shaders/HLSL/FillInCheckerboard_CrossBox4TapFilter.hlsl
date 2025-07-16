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

// Desc: Filters/fills-in invalid values for a checkerboard filled input from neighborhood.
// The compute shader is to be run with (width, height / 2) dimensions as 
// it scales Y coordinate by 2 to process only the inactive pixels in the checkerboard filled input.

#ifndef __FILLINCHECKERBOARD_CROSSBOX4TAPFILTER_HLSL__
#define __FILLINCHECKERBOARD_CROSSBOX4TAPFILTER_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"

RWTexture2D<float2> gio_Values : register(u0);

ConstantBuffer<ConstantBuffers::SVGF::CalcLocalMeanVarianceCB> cbLocalMeanVar : register(b0);

// Adjust an index in Y coordinate to a same/next pixel that has an invalid value generated for it.
int2 GetInactivePixelIndex(int2 pixel) {
    const bool IsEvenPixel = ((pixel.x + pixel.y) & 1) == 0;
    return cbLocalMeanVar.EvenPixelActivated == IsEvenPixel ? pixel + int2(0, 1) : pixel;
}

[numthreads(
    ShadingConvention::SVGF::ThreadGroup::Default::Width,
    ShadingConvention::SVGF::ThreadGroup::Default::Height,
    ShadingConvention::SVGF::ThreadGroup::Default::Depth)]
void CS(uint2 DTid : SV_DispatchThreadID) {
    const int2 Pixel = GetInactivePixelIndex(int2(DTid.x, DTid.y * 2));

    // Load 4 valid neighbors.
    const int2 SrcIndexOffsets[4] = { {-1, 0}, {0, -1}, {1, 0}, {0, 1} };
    float4x2 inValues_4x2;
    {
        [unroll]
        for (uint i = 0; i < 4; i++)
            inValues_4x2[i] = gio_Values[Pixel + SrcIndexOffsets[i]];
    }

    // Average valid inputs.
    const float4 Weights = inValues_4x2._11_21_31_41 != ShadingConvention::RTAO::InvalidAOCoefficientValue;
    const float WeightSum = dot(1, Weights);
    const float2 FilteredValue =  WeightSum > 1e-3 ? mul(Weights, inValues_4x2) / WeightSum : ShadingConvention::RTAO::InvalidAOCoefficientValue;

    gio_Values[Pixel] = FilteredValue;
}

#endif // __FILLINCHECKERBOARD_CROSSBOX4TAPFILTER_HLSL__