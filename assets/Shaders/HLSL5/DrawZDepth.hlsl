#ifndef __DRAWZDEPTH_HLSL__
#define __DRAWZDEPTH_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef HLSL_VERSION_UNDER_6
#define HLSL_VERSION_UNDER_6
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL5/Samplers.hlsli"

LightCB_register(b0);
ObjectCB_register(b1);
MaterialCB_register(b2);

VERTEX_IN

struct VertexOut {
    float4 PosW : SV_Position;
    float2 TexC : TEXCOORD0;
};

struct GeoOut {
    float4 PosH : SV_Position;
    float2 TexC : TEXCOORD0;
    uint ArrayIndex : SV_RenderTargetArrayIndex;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    
    vout.TexC = vin.TexC;
    
    float4 posW = mul(float4(vin.PosL, 1.f), World);
    vout.PosW = posW;
    
    return vout;
}

[maxvertexcount(18)]
void GS(in triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream) {
    GeoOut gout = (GeoOut) 0;

	// Directional light or spot light
    if (Light.Type == Common::Foundation::LightType_Directional || Light.Type == Common::Foundation::LightType_Spot) {
        gout.ArrayIndex = 0;
        
		[unroll]
        for (uint i = 0; i < 3; ++i) {
            gout.PosH = mul(gin[i].PosW, Light.Mat0);
            gout.TexC = gin[i].TexC;

            triStream.Append(gout);
        }
    }
	// Point light
    //else if (_Light.Type == LightType_Point) {
	//	[loop]
    //    for (uint face = 0; face < 6; ++face) {
    //        gout.ArrayIndex = face;
    //
    //        const float4x4 ViewProj = Shadow::GetViewProjMatrix(_Light, face);
    //
	//		[unroll]
    //        for (uint i = 0; i < 3; ++i) {
    //            gout.PosH = mul(gin[i].PosW, ViewProj);
    //            gout.TexC = gin[i].TexC;
    //
    //            triStream.Append(gout);
    //        }
    //
    //        triStream.RestartStrip();
    //    }
    //}
}

void PS(GeoOut pin) {
    clip(Albedo.a - 0.1f);
}

#endif // __DRAWZDEPTH_HLSL__