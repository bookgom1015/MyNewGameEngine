#ifndef __TONEMAPPING_HLSL__
#define __TONEMAPPING_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Common/Render/TonemapperType.h"
#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

ToneMapping_Default_RootConstants(b0)

Texture2D<ShadingConvention::ToneMapping::IntermediateMapFormat> gi_IntermediateMap : register(t0);
RWStructuredBuffer<float> gi_Luminance : register(u0);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

float3 TonemapExponential(in float3 hdr) {
    return (float3)1.f - exp(-hdr);
}

float3 TonemapReinhard(in float3 hdr, in float k = 1.f) {
    return hdr / (hdr + k);
}

float3 TonemapReinhardExt(in float3 hdr, in float wp = 1.f) {
    const float3 numer = hdr * (1.f + hdr / (wp * wp));
    const float3 denom = 1.f + hdr;
    return numer / denom;
}

float3 TonemapHableParital(in float3 x) {
    const float A = 0.15f;
    const float B = 0.50f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.02f;
    const float F = 0.30f;
    
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 TonemapHable(in float3 hdr) {
    const float ExposureBias = 2.f;
    const float3 Color = TonemapHableParital(hdr * ExposureBias);

    const float3 White = (float3)11.2f; 
    const float3 whiteScale = 1.f / TonemapHableParital(White);

    return Color * whiteScale;
}

float3 TonemapUncharted2(in float3 hdr) {
    float3 color = TonemapHable(hdr);
    
    const float3 White = (float3)11.2f;    
    color *= 1.f / TonemapHable(White);
    
    return color;
}

float3 TonemapACES(in float3 hdr) {
    const float A = 2.51f;
    const float B = 0.03f;
    const float C = 2.43f;
    const float D = 0.59f;
    const float E = 0.14f;
    return saturate((hdr * (A * hdr + B)) / (hdr * (C * hdr + D) + E));
}

float3 TonemapLog(in float3 hdr, in float a = 1.f) {
    return log(1.f + a * hdr) / log(1.f + a);
}

SDR_FORMAT PS(in VertexOut pin) : SV_Target {
    const float3 HDR = gi_IntermediateMap.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
        
    const float Luminance = gi_Luminance[0];
    const float Exposure = exp2(-Luminance) * gMiddleGrayKey;
        
    const float3 Color = HDR * (Exposure);
    
    float3 sdr = (float3)0.f;
    if (gTonemapperType == Common::Render::TonemapperType::E_Exponential)
        sdr = TonemapExponential(Color);
    else if (gTonemapperType == Common::Render::TonemapperType::E_Reinhard)
        sdr = TonemapReinhard(Color);
    else if (gTonemapperType == Common::Render::TonemapperType::E_ReinhardExt)
        sdr = TonemapReinhardExt(Color);
    else if (gTonemapperType == Common::Render::TonemapperType::E_Uncharted2)
        sdr = TonemapUncharted2(Color);
    else if (gTonemapperType == Common::Render::TonemapperType::E_ACES)
        sdr = TonemapACES(Color);
    else if (gTonemapperType == Common::Render::TonemapperType::E_Log)
        sdr = TonemapLog(Color);

    return float4(sdr, 1.f);
}
                                    
#endif // __TONEMAPPING_HLSL__