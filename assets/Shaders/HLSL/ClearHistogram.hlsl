#ifndef __CLEARHISTOGRAM_HLSL__
#define __CLEARHISTOGRAM_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

EyeAdaption_LuminanceHistogram_RootConstants(b0)

Texture2D<HDR_FORMAT> gi_BackBuffer	: register(t0);
RWStructuredBuffer<ShadingConvention::EyeAdaption::HistogramBin> go_Histogram : register(u0);

[numthreads(MAX_BIN_COUNT, 1, 1)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    if (DTid.x < gBinCount) go_Histogram[DTid.x].Count = 0;
}

#endif // __CLEARHISTOGRAM_HLSL__