#ifndef __CONVERTEQUIRECTANGULARTOCUBEMAP_HLSL__
#define __CONVERTEQUIRECTANGULARTOCUBEMAP_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _CUBE_COORD
#define _CUBE_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

#include "./../../../assets/Shaders/HLSL/EquirectangularConverter.hlsli"

ConstantBuffer<ConstantBuffers::ProjectToCubeCB> cbProjectToCube : register(b0);

Texture2D<float4> gi_EquirectangularMap : register(t0);

struct VertexOut {
    float3 PosL : POSITION;
};

struct GeoOut {
    float4 PosH     : SV_Position;
    float3 PosL     : POSITION;
    uint ArrayIndex : SV_RenderTargetArrayIndex;
};

VertexOut VS(in uint vid : SV_VertexID) {
    VertexOut vout = (VertexOut)0;

    vout.PosL = gVertices[vid];

    return vout;
}

[maxvertexcount(18)]
void GS(in triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream) {
    GeoOut gout = (GeoOut)0;

	[unroll]
    for (uint face = 0; face < 6; ++face) {
        gout.ArrayIndex = face;
        
        const float4x4 View = cbProjectToCube.View[face];
        
		[unroll]
        for (uint i = 0; i < 3; ++i) {
            const float3 PosL = gin[i].PosL;
            const float4 PosV = mul(float4(PosL, 1), View);
            const float4 PosH = mul(PosV, cbProjectToCube.Proj);

            gout.PosL = PosL;
            gout.PosH = PosH.xyww;

            triStream.Append(gout);
        }
        
        triStream.RestartStrip();
    }
}

float4 PS(in GeoOut pin) : SV_Target {
    const float2 TexC = EquirectangularConverter::SampleSphericalMap(normalize(pin.PosL));
    const float3 Color = gi_EquirectangularMap.SampleLevel(gsamLinearClamp, TexC, 0).rgb;
    
    return float4(Color, 1.f);
}

#endif // __CONVERTEQUIRECTANGULARTOCUBEMAP_HLSL__