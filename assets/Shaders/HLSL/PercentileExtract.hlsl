#ifndef __PERCENTILEEXTRACT_HLSL__
#define __PERCENTILEEXTRACT_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

RWStructuredBuffer<ShadingConvention::EyeAdaption::HistogramBin> gi_Histogram : register(u0);
RWStructuredBuffer<ShadingConvention::EyeAdaption::Result> go_Output : register(u1);

float BinToLogLumCenter(in uint bin) {
    float t = (bin + 0.5f) / (float)gBinCount;
    return gMinLogLum + t * (gMaxLogLum - gMinLogLum);
}

[numthreads(1, 1, 1)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    uint total = 0;
    [unroll]
    for (uint i = 0; i < gBinCount; ++i)
        total += gi_Histogram[i].Count;
    
    if (total == 0) {
        ShadingConvention::EyeAdaption::Result result;
        result.AvgLogLum = 0.f;
        result.LowLogLum = 0.f;
        result.HighLogLum = 0.f;
        result.LowBin = 0;
        result.HighBin = 0;
        result.TotalCount = 0;
        go_Output[0] = result;        
        return;
    }
    
    uint lowTarget = (uint)ceil(total * gLowPercent);
    uint highTarget = (uint)ceil(total * gHighPercent);
    
    lowTarget = min(lowTarget, total);
    highTarget = min(highTarget, total);
    
    uint lowBin = 0;
    uint highBin = gBinCount - 1;
    
    uint cdf = 0;
    [unroll]
    for (uint i = 0; i < gBinCount; ++i) {
        cdf += gi_Histogram[i].Count;
        if (cdf >= lowTarget) { lowBin = i; break; }
    }
    
    cdf = 0;
    [unroll]
    for (uint i = 0; i < gBinCount; ++i) {
        cdf += gi_Histogram[i].Count;
        if (cdf >= highTarget) { highBin = i; break; }
    }
    
    if (lowBin > highBin) {
        uint temp = lowBin;
        lowBin = highBin;
        highBin = temp;
    }
    
    float weightedSum = 0.f;
    uint usedCount = 0;
    
    [unroll]
    for (uint i = lowBin; i <= highBin; ++i) {
        const uint c = gi_Histogram[i].Count;
        usedCount += c;
        
        const float LogLumCenter = BinToLogLumCenter(i);
        weightedSum += c * LogLumCenter;
    }
    
    const float AvgLogLum = (usedCount > 0) ? (weightedSum / (float)usedCount)
        : BinToLogLumCenter((lowBin + highBin) >> 1);
    
    ShadingConvention::EyeAdaption::Result result;
    result.AvgLogLum = AvgLogLum;
    result.LowLogLum = BinToLogLumCenter(lowBin);
    result.HighLogLum = BinToLogLumCenter(highBin);
    result.LowBin = lowBin;
    result.HighBin = highBin;
    result.TotalCount = total;
    go_Output[0] = result;
}

#endif // __PERCENTILEEXTRACT_HLSL__